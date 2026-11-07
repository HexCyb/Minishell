# minishell

[![CI](https://github.com/HexCyb/Minishell/actions/workflows/ci.yml/badge.svg)](https://github.com/HexCyb/Minishell/actions/workflows/ci.yml)

A small Bash-like shell in C: a prompt with history, pipes, redirections and here-documents, quotes, `$VAR` and
`$?`, the usual builtins, `&&`, `||`, parentheses and `*` wildcards, with `Ctrl-C`, `Ctrl-D` and `Ctrl-\` handled
like in bash.

> Originally a pair project at **42 Türkiye** (common core). This repository is a 2026 rewrite.
> 42 Türkiye'de ikili proje olarak tamamlandı; bu depo 2026'da yeniden yazılmış hali.

```sh
make            # needs GNU readline (Debian/Ubuntu: libreadline-dev)
./minishell
```

```
minishell$ export NAME=42
minishell$ echo "hello $NAME" | tr a-z A-Z > out.txt && cat out.txt
HELLO 42
minishell$ cat << EOF | wc -l
> one
> two
> EOF
2
minishell$ (cd /tmp && false) || echo "status: $?, still in $PWD"
status: 1, still in /home/taha
minishell$ nope
minishell: nope: command not found
minishell$ echo $?
127
```

## What it does

| | |
|---|---|
| **Prompt** | line editing and history (readline); without a terminal it reads a script from stdin |
| **Pipes** | `cmd1 \| cmd2 \| … \| cmdN`, every command in its own process |
| **Redirections** | `<`, `>`, `>>` and here-documents `<<`, in any number and position: `> out echo hi` |
| **Quotes** | `'…'` keeps everything as typed, `"…"` still expands `$` |
| **Expansions** | `$VAR`, `$?`; unquoted results are split into words, like bash |
| **Lists** | `&&`, `\|\|` and `( … )` for grouping, run in a subshell |
| **Wildcards** | `*` matches names in the current directory (hidden ones only with a leading `.`) |
| **Builtins** | `echo -n`, `cd` (`cd`, `cd -`), `pwd`, `export` (`NAME`, `NAME=v`, `NAME+=v`), `unset`, `env`, `exit` |
| **Signals** | `Ctrl-C` gives a fresh prompt (`$?` = 130), `Ctrl-D` exits, `Ctrl-\` does nothing at the prompt and stops a running command (`Quit`, 131) |
| **Errors** | bash's messages and statuses: `syntax error near unexpected token`, 127 not found, 126 not executable |

`;` and `\` are not special, as the subject asks, and an unclosed quote is a syntax error.

## How a line runs

```
"echo $A | wc -c > n"
   │  lexer.c       words and operators, quotes kept:   echo │ $A │ | │ wc │ -c │ > │ n
   │  parser.c      syntax tree:                         PIPE(CMD echo $A, CMD wc -c >n)
   │  heredoc.c     every << is read now, in the order typed
   │  expand.c      per command, just before it runs:   $A → value → split → quotes removed → *
   │  redirect.c    open the files, dup2 onto stdin / stdout
   ▼  exec.c        builtin in the shell, or fork + execve; pipeline.c connects the processes
      wait.c        the last command's status becomes $?
```

The parser is a small recursive descent over this grammar:

```
list      := pipeline { ("&&" | "||") pipeline }
pipeline  := command { "|" command }
command   := "(" list ")" { redirection }  |  { word | redirection }+
```

## Design choices

- **Expansion keeps a "quoted" flag per character.** After `$` expansions and quote removal, every character
  remembers whether it was quoted. Word splitting only cuts at unquoted blanks and `*` only matches when it was
  not quoted, so `"$A"` stays one word, `$A` may become several, and `"*"` is a star.
- **Builtins run in the shell itself** when they are alone (so `cd` and `export` change the shell), with their
  redirections undone afterwards. In a pipeline they run in the child, like in bash: `cd / | cat` changes nothing.
- **Here-documents go to a file that is unlinked at once.** The text stays readable through the descriptor, any
  size works (a pipe would block after 64 KiB), and nothing is left in `/tmp`.
- **Without a terminal, input is read one byte at a time**, so a command can read the rest of the script
  itself, as under bash: `printf 'cat\nhello\n' | ./minishell` prints `hello`.
- **One global variable**, the number of the last signal. At the prompt `Ctrl-C` clears the line through
  readline; while children run, the shell ignores the signals and the children get the default action.
- **Every process frees its memory before it leaves**, forked children included (Valgrind checks them all).

## Tests

`make test` runs [`tests/run_tests.py`](tests/run_tests.py), 126 checks:

- **105 scripts run under both bash and minishell**, each in a fresh folder with the same files. stdout and the
  exit status must be identical, and so must stderr once `bash: line N:` is read as `minishell:`. They cover
  quotes, expansions and word splitting, `exit`, `PATH` search and its errors, every redirection, pipes,
  here-documents, `&&` `||` `( )`, wildcards, the builtins and syntax errors.
- **11 cases where minishell differs from bash on purpose** (unclosed quotes, `;`, …), checked against fixed answers.
- **10 checks in a pseudo-terminal**, as a person would type: the prompt, `Ctrl-C` on a line, on a running command
  and in a here-document, `Ctrl-\`, history with the arrow keys, `Ctrl-D`.

`make valgrind` runs every script again under Valgrind: no leak and no memory error in any process.
[CI](.github/workflows/ci.yml) builds with gcc and clang, runs the tests, runs them again with AddressSanitizer and
UndefinedBehaviorSanitizer, then under Valgrind.
