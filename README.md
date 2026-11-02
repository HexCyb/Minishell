> Developed together with **Burak Yılmaz** ([@Ropnez](https://github.com/Ropnez)) at 42 Türkiye · original repository: [Ropnez/Minishell](https://github.com/Ropnez/Minishell)
>
> Bu proje 42 Türkiye'de Burak Yılmaz ile birlikte geliştirildi.

# Minishell

A small Bash-like shell written in C. It has a prompt with history, pipes, redirections, here-documents, quotes, environment variables and the usual builtins, and it handles `Ctrl-C`, `Ctrl-D` and `Ctrl-\` like Bash does.

## Build and run

```sh
make            # needs the GNU readline library
./minishell
```

```
minishell$ export NAME=42
minishell$ echo "hello $NAME" | tr a-z A-Z > out.txt
minishell$ cat << EOF | wc -l
> one
> two
> EOF
2
minishell$ ls nope; echo $?
ls: nope: No such file or directory
1
```

## Features

| | |
|---|---|
| **Prompt** | Line editing and history (readline) |
| **Pipes** | `cmd1 \| cmd2 \| ... \| cmdN`, each command in its own process |
| **Redirections** | `<` input, `>` output, `>>` append, `<<` here-document |
| **Quotes** | `'single'` keeps everything literally, `"double"` still expands `$` |
| **Expansions** | `$VAR` from the shell's own environment and `$?`, the exit status of the last command |
| **Builtins** | `echo` (with `-n`), `cd`, `pwd`, `export`, `unset`, `env`, `exit` |
| **Signals** | `Ctrl-C` gives a fresh prompt, `Ctrl-D` exits, `Ctrl-\` does nothing at the prompt |
| **Errors** | Bash-style messages, for example `syntax error near unexpected token` |

## How a line is executed

1. **Syntax check** (`syntax/`): unclosed quotes and misplaced `|`, `<`, `>` are rejected before anything runs.
2. **Here-documents** (`src/handle_heredoc.c`) are read first, so their text is ready when the commands start.
3. **Split** (`src/split_commands.c`): the line is cut at every `|` that is not inside quotes.
4. **Expand** (`expansions/`): `$VAR` and `$?` are replaced and quotes are removed.
5. **Redirect** (`redirects/`): files are opened and connected to the command's stdin and stdout.
6. **Execute** (`executes/`): a single builtin runs inside the shell itself (so `cd` and `export` change the shell). Everything else is found in `PATH` and run with `fork` + `execve`, connected by pipes.
7. **Wait** (`executes/wait.c`): the exit status of the last command becomes `$?`.

## Project structure

```
src/          main loop, prompt, here-docs, splitting the line
syntax/       syntax checks
expansions/   $VAR and $?
redirects/    <  >  >>  <<
executes/     one command, pipelines, PATH search, waiting
builtins/     echo cd pwd export unset env exit
minienv/      the shell's own copy of the environment (a linked list)
utils/        signals, quotes, strings, errors
includes/     headers
```
