#!/usr/bin/env python3
"""Tests for minishell.

1. Every script below runs twice, under bash and under ./minishell, in a fresh folder with the same files.
   stdout and the exit status must be identical; stderr must be too, once "bash: line N:" is read as
   "minishell:" (syntax errors: first line only, bash adds a second one).
2. A few cases where minishell and bash differ on purpose are checked against fixed answers.
3. Interactive behaviour (prompt, Ctrl-C, Ctrl-D, Ctrl-\\, history, here-document interrupted) runs in a
   pseudo-terminal.

    make test
    python3 tests/run_tests.py --valgrind     # every script again under Valgrind: no leak, no error
"""
import os
import pty
import re
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MINISHELL = ROOT / "minishell"
SUPP = ROOT / "tests" / "readline.supp"
FAILURES = []
VALGRIND = "--valgrind" in sys.argv

# (name, script, how stderr is compared: "full", "first" line, or "skip")
COMPARE = [
    # echo and quotes
    ("echo", "echo hello world", "full"),
    ("echo -n", "echo -n hi\necho", "full"),
    ("echo -nnn -n", "echo -nnnn -n -n hi\necho", "full"),
    ("echo -n then text", "echo -n -x hi\necho -na b", "full"),
    ("empty arguments", "echo '' \"\" x\necho \"\"", "full"),
    ("blanks between words", "echo a     b\t\tc", "full"),
    ("single quotes keep all", "echo '$HOME \"x\" $?'", "full"),
    ("double quotes expand", "echo \"home: $HOME, 'q' \"", "full"),
    ("quotes glued together", "echo \"a\"'b'c\"\"'d'", "full"),
    ("$ before quotes", "echo $\"HOME\" $'x'", "full"),
    ("lonely $", "echo $ \"$\" a$ $= x", "full"),
    # variables
    ("export and use", "export X=42\necho $X \"$X\" '$X'", "full"),
    ("unset variable", "echo [$NOPE] \"[$NOPE]\"", "full"),
    ("empty expansion disappears", "echo $NOPE hi $NOPE\n$NOPE\necho $?", "full"),
    ("word splitting", "export A=\"1 2   3\"\necho $A\necho \"$A\"\necho x$A\"y\"", "full"),
    ("splitting keeps globs", "export S='*.c'\necho $S \"$S\"", "full"),
    ("$? after commands", "false\necho $?\ntrue\necho $?\nnope_command\necho $?", "full"),
    ("$_ and digits", "echo $1 $9x [$_NONE]", "full"),
    ("export +=", "export P=ab\nexport P+=cd Q+=new\necho $P $Q", "full"),
    ("export without value", "export LATER\nenv | grep -c LATER\nexport | grep LATER", "full"),
    ("export invalid", "export 1A=b OK=1 =x A-B=2\necho $? $OK", "full"),
    ("unset", "export U=1\nunset U NOPE 1A\necho [$U] $?", "full"),
    ("export sorted listing", "export | grep -v -e GLIBC -e 'x LD_' | head -n 4", "full"),  # Valgrind adds those
    ("SHLVL goes up", "echo $SHLVL", "full"),
    # exit
    ("exit 42", "exit 42\necho never", "full"),
    ("exit 300", "exit 300", "full"),
    ("exit -1", "exit -1", "full"),
    ("exit with blanks", "exit \" 7 \"", "full"),
    ("exit not a number", "exit abc", "full"),
    ("exit too big", "exit 9223372036854775808", "full"),
    ("exit too many", "exit 1 2\necho still here $?", "full"),
    ("exit keeps $?", "false\nexit", "full"),
    ("exit inside a pipe", "exit 5 | echo after\necho $?", "full"),
    # commands and paths
    ("absolute path", "/bin/echo absolute", "full"),
    ("command not found", "nope_command\necho $?", "full"),
    ("empty command", "\"\"\necho $?", "full"),
    ("no such file", "./nope\necho $?", "full"),
    ("is a directory", "/tmp\necho $?\n./sub\necho $?", "full"),
    ("permission denied", "./script.sh\necho $?", "full"),
    ("not a directory", "./file1/x\necho $?", "full"),
    ("PATH unset", "unset PATH\nls\necho $?", "full"),
    ("PATH search order", "export PATH=/nowhere:/bin:/usr/bin\nls file1", "full"),
    ("arguments with spaces", "printf '[%s]\\n' a 'b c' \"d  e\"", "full"),
    # redirections
    ("> and <", "echo hi > out\ncat < out\ncat out", "full"),
    (">>", "echo one > out\necho two >> out\ncat out", "full"),
    ("redirection first", "> out echo first\n< out cat", "full"),
    ("several outputs", "echo x > o1 > o2\ncat o1 o2\nls o1 o2", "full"),
    ("several inputs", "cat < file1 < c.txt", "full"),
    ("only a redirection", "> created\nls created\n< file1\necho $?", "full"),
    ("missing input file", "cat < missing\necho $?\n< missing echo hi\necho $?", "full"),
    ("ambiguous redirect", "export TWO='a b'\necho hi > $TWO\necho $?\necho hi > $NOPE\necho $?", "full"),
    ("redirect expanded name", "export F=named\necho x > $F\ncat named", "full"),
    ("builtin with redirection", "pwd > p\necho x >> p\ncat p | wc -l\nexport > e\ngrep -c HOME e", "full"),
    # pipes
    ("simple pipe", "echo hi | cat", "full"),
    ("long pipe", "cat file1 | sort -r | head -n 2 | tr a-z A-Z", "full"),
    ("status of a pipe", "echo hi | false\necho $?\nfalse | echo ok\necho $?", "full"),
    ("pipe and SIGPIPE", "yes | head -n 3\necho $?", "full"),
    ("builtins in pipes", "export | grep -c HOME\npwd | cat\necho a b | wc -w", "full"),
    ("cd in a pipe changes nothing", "cd / | cat\npwd | grep -c /", "full"),
    ("commands read the script", "cat\nthis line goes to cat\n", "full"),
    # here-documents
    ("here-doc", "cat << EOF\nline 1\n  line 2\nEOF", "full"),
    ("here-doc expands", "export N=42\ncat << EOF\nn=$N '$N' \"$N\" $?\nEOF", "full"),
    ("quoted delimiter", "export N=42\ncat << 'EOF'\nn=$N\nEOF\ncat << \"E\"F\n$N\nEF", "full"),
    ("two here-docs", "cat << A << B\nfrom a\nA\nfrom b\nB", "full"),
    ("here-doc in a pipe", "cat << END | wc -l\n1\n2\n3\nEND", "full"),
    ("here-docs in order", "cat << A | cat - << B\na\nA\nb\nB", "full"),
    ("here-doc without command", "<< X\nignored\nX\necho $?", "full"),
    # && || ( )
    ("&& ||", "true && echo a || echo b\nfalse && echo a || echo b", "full"),
    ("|| chain", "false || false || echo third\necho $?", "full"),
    ("$? between steps", "false || echo status $?", "full"),
    ("subshell", "(echo in && exit 3) || echo out $?", "full"),
    ("subshell keeps cd", "(cd / && pwd) && pwd | grep -c '^/$'", "full"),
    ("subshell redirection", "(echo a && echo b) > both\ncat both", "full"),
    ("subshell in pipe", "(echo x && echo y) | wc -l", "full"),
    ("nested parentheses", "((echo deep) && (false || echo deeper))", "full"),
    ("precedence", "false && echo no || echo yes && echo also", "full"),
    # wildcards
    ("star", "echo *", "full"),
    ("star suffix", "echo *.c", "full"),
    ("star prefix", "echo f*", "full"),
    ("star middle", "echo f*1 *.t*", "full"),
    ("no match stays", "echo *.none", "full"),
    ("quoted star", "echo \"*\" '*'.c", "full"),
    ("hidden files", "echo .h* .*", "full"),
    ("star as argument", "ls *.c | wc -l", "full"),
    # builtins
    ("pwd", "pwd", "full"),
    ("cd absolute", "cd /tmp\npwd", "full"),
    ("cd home", "cd\npwd", "full"),
    ("cd relative", "cd sub\npwd\ncd ..\npwd", "full"),
    ("cd -", "cd /\ncd -", "full"),
    ("cd errors", "cd nope\necho $?\ncd a b\necho $?", "full"),
    ("cd updates PWD", "cd sub\necho $PWD $OLDPWD", "full"),
    ("cd HOME not set", "unset HOME\ncd\necho $?", "full"),
    ("env", "env | grep -c '^PATH='\nenv | grep -c '^HOME='", "full"),
    ("killed by a signal", "sh -c 'kill -TERM $$'\necho $?", "full"),
    ("killed in a pipe", "sh -c 'kill -TERM $$' | cat\necho $?\ncat /dev/null | sh -c 'kill -TERM $$'\necho $?",
     "full"),
    # syntax errors: nothing runs, status 2 (a script under bash stops there: one line per test;
    # bash adds the line itself on a second line of stderr)
    ("pipe at the start", "| echo a", "first"),
    ("redirection without file", "echo a >", "first"),
    ("two redirections", "echo a > >", "first"),
    ("here-doc without delimiter", "cat <<", "first"),
    ("unexpected )", "echo a)", "first"),
    ("empty parentheses", "()", "first"),
    ("word after )", "(echo a) b", "first"),
    ("&& at the start", "&& echo a", "first"),
    ("|| twice", "echo a || || echo b", "first"),
    ("| then &&", "echo a | && echo b", "first"),
]

# where minishell and bash differ on purpose: (name, script, stdout, status, stderr part)
EXPECT = [
    ("unclosed quote", "echo 'abc\necho $?", "2\n", 0, "minishell: syntax error: unclosed quote"),
    ("syntax error, then the next line", "echo a | | b\necho $?", "2\n", 0,
     "minishell: syntax error near unexpected token `|'"),
    ("pipe at the end", "echo a |", "", 2, "syntax error near unexpected token `newline'"),
    ("unclosed parenthesis", "(echo a", "", 2, "syntax error near unexpected token `newline'"),
    ("( after a word", "echo (a)", "", 2, "syntax error near unexpected token `('"),
    ("unclosed double quote", "echo \"abc", "", 2, "unclosed quote"),
    ("; is not special", "echo a; echo b", "a; echo b\n", 0, ""),
    ("\\ is not special", "echo a\\b", "a\\b\n", 0, ""),
    ("$0", "echo $0", "minishell\n", 0, ""),
    ("env with arguments", "env x", "", 1, "minishell: env:"),
    ("here-doc ends at EOF", "cat << END\nonly line", "only line\n", 0,
     "warning: here-document delimited by end-of-file (wanted `END')"),
]

FILES = {"a.c": "", "b.c": "", "c.txt": "text\n", "file1": "one\ntwo\nthree\n", ".hidden": "", "script.sh": "echo x\n"}


def check(name, ok, detail=""):
    print(f"  {'ok  ' if ok else 'FAIL'}  {name}" + ("" if ok else f"\n        {detail}"))
    if not ok:
        FAILURES.append(name)


def make_folder():
    d = Path(tempfile.mkdtemp(prefix="ms_"))
    for name, text in FILES.items():
        (d / name).write_text(text)
    os.chmod(d / "script.sh", 0o644)
    (d / "sub").mkdir()
    (d / "home").mkdir()
    return d


def run(cmd, script, folder):
    env = {"PATH": "/usr/local/bin:/usr/bin:/bin", "HOME": str(folder / "home"), "LC_ALL": "C", "USER": "tester",
           "SHLVL": "1"}
    proc = subprocess.run(cmd, input=script.encode(), capture_output=True, cwd=folder, env=env, timeout=60)
    return proc.stdout.decode(errors="replace"), proc.stderr.decode(errors="replace"), proc.returncode


def normalize(err):
    err = re.sub(r"^bash: line \d+: ", "minishell: ", err, flags=re.M)
    return err.replace(tempfile.gettempdir(), "TMP")


def ms_command():
    if not VALGRIND:
        return [str(MINISHELL)]
    return ["valgrind", "-q", "--leak-check=full", "--show-leak-kinds=definite,indirect",
            "--errors-for-leak-kinds=definite,indirect", "--error-exitcode=42", "--track-fds=no",
            f"--suppressions={SUPP}", str(MINISHELL)]


def compare_with_bash():
    print("== the same scripts under bash and minishell")
    for name, script, mode in COMPARE:
        f1, f2 = make_folder(), make_folder()
        try:
            b_out, b_err, b_code = run(["bash"], script + "\n", f1)
            m_out, m_err, m_code = run(ms_command(), script + "\n", f2)
        finally:
            shutil.rmtree(f1)
            shutil.rmtree(f2)
        b_out = b_out.replace(str(f1), "DIR")
        m_out = m_out.replace(str(f2), "DIR")
        b_err, m_err = normalize(b_err.replace(str(f1), "DIR")), normalize(m_err.replace(str(f2), "DIR"))
        if VALGRIND:
            m_err = re.sub(r"^==\d+==.*\n?", "", m_err, flags=re.M)
            if m_code == 42 and b_code != 42:
                check(name, False, f"valgrind reported an error: {m_err!r}")
                continue
        if mode == "first":
            b_err, m_err = b_err.split("\n")[0], m_err.split("\n")[0]
        elif mode == "skip":
            b_err = m_err = ""
        same = (b_out, b_code, b_err) == (m_out, m_code, m_err)
        check(name, same, f"bash:      out={b_out!r} status={b_code} err={b_err!r}\n"
                          f"        minishell: out={m_out!r} status={m_code} err={m_err!r}")


def fixed_answers():
    print("\n== where minishell differs from bash on purpose")
    for name, script, out, code, err_part in EXPECT:
        folder = make_folder()
        try:
            m_out, m_err, m_code = run(ms_command(), script + "\n", folder)
        finally:
            shutil.rmtree(folder)
        if VALGRIND:
            m_err = re.sub(r"^==\d+==.*\n?", "", m_err, flags=re.M)
        check(name, (m_out, m_code) == (out, code) and err_part in m_err,
              f"out={m_out!r} status={m_code} err={m_err!r}")


class Terminal:
    """minishell in a pseudo-terminal, as a person would use it."""

    def __init__(self, folder):
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            os.chdir(folder)
            os.execve(str(MINISHELL), [str(MINISHELL)], {"PATH": "/usr/bin:/bin", "HOME": str(folder),
                                                          "TERM": "xterm", "LC_ALL": "C"})
        self.buf = b""

    def read_until(self, pattern, timeout=5):
        end = time.time() + timeout
        while time.time() < end:
            if re.search(pattern, self.buf):
                found = self.buf
                self.buf = b""
                return found.decode(errors="replace")
            ready, _, _ = select.select([self.fd], [], [], 0.1)
            if ready:
                try:
                    data = os.read(self.fd, 4096)
                except OSError:
                    break
                if not data:
                    break
                self.buf += data
        found = self.buf.decode(errors="replace")
        self.buf = b""
        return None if not re.search(pattern, found.encode()) else found

    def send(self, text):
        os.write(self.fd, text.encode())

    def status(self, timeout=5):
        end = time.time() + timeout
        while time.time() < end:
            pid, st = os.waitpid(self.pid, os.WNOHANG)
            if pid:
                return os.waitstatus_to_exitcode(st)
            self.read_until(rb"$^", 0.1)
        os.kill(self.pid, signal.SIGKILL)
        os.waitpid(self.pid, 0)
        return None


def interactive():
    print("\n== interactive (pseudo-terminal)")
    folder = make_folder()
    t = Terminal(folder)
    prompt = rb"minishell\$ "
    check("prompt", t.read_until(prompt) is not None)
    t.send("echo hi there\n")
    check("a command runs", t.read_until(rb"hi there\r?\n.*minishell\$ ") is not None)
    t.send("abc\x03")
    check("Ctrl-C gives a new prompt", t.read_until(rb"\n.*minishell\$ ") is not None)
    t.send("echo status=$?\n")
    check("... and $? is 130", t.read_until(rb"status=130") is not None)
    t.send("\x1c")
    t.send("echo quit ignored\n")
    check("Ctrl-\\ at the prompt does nothing", t.read_until(rb"quit ignored\r?\n.*minishell\$ ") is not None)
    t.send("cat\n")
    time.sleep(0.3)
    t.send("\x03")
    t.read_until(prompt)
    t.send("echo cat=$?\n")
    check("Ctrl-C stops a command: 130", t.read_until(rb"cat=130") is not None)
    t.send("sleep 5\n")
    time.sleep(0.3)
    t.send("\x1c")
    quit_msg = t.read_until(rb"Quit")
    t.read_until(prompt)
    t.send("echo sleep=$?\n")
    check("Ctrl-\\ stops a command: Quit, 131", quit_msg is not None and t.read_until(rb"sleep=131") is not None)
    t.send("cat << EOF\n")
    t.read_until(rb"> ")
    t.send("half\x03")
    t.read_until(prompt)
    t.send("echo heredoc=$?\n")
    check("Ctrl-C in a here-document: back to the prompt, 130", t.read_until(rb"heredoc=130") is not None)
    t.send("echo one\n")
    t.read_until(prompt)
    t.send("\x1b[A\n")
    check("history: arrow up runs the last line again", t.read_until(rb"(?s)echo one\r\n.*one\r\n") is not None)
    t.read_until(prompt)
    t.send("false\n")
    t.read_until(prompt)
    t.send("\x04")
    out = t.read_until(rb"exit")
    code = t.status()
    check("Ctrl-D prints exit and leaves with the last status", out is not None and code == 1, f"{out!r} {code}")
    shutil.rmtree(folder)


def main():
    if not MINISHELL.exists():
        sys.exit("./minishell is missing: run make")
    compare_with_bash()
    fixed_answers()
    if not VALGRIND:
        interactive()
    print()
    if FAILURES:
        print(f"{len(FAILURES)} check(s) failed")
        sys.exit(1)
    print("all checks passed")


if __name__ == "__main__":
    main()
