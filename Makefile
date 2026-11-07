NAME     := minishell
CC       := cc
CFLAGS   := -Wall -Wextra -Werror -Iinclude
LDLIBS   := -lreadline

SRC      := src/main.c src/input.c src/signals.c src/lexer.c src/parser.c \
            src/tree.c src/heredoc.c src/buf.c src/expand.c \
            src/expand_dollar.c src/fields.c src/wildcard.c src/redirect.c \
            src/exec.c src/pipeline.c src/external.c src/path.c src/wait.c \
            src/builtins.c src/builtin_simple.c src/builtin_cd.c \
            src/builtin_export.c src/builtin_exit.c src/env.c src/env_list.c \
            src/str.c src/str2.c src/mem.c src/error.c
OBJ      := $(SRC:src/%.c=obj/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDLIBS) -o $@

obj/%.o: src/%.c include/minishell.h Makefile
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

# The bonus (&&, ||, parentheses, wildcards) is part of the same program.
bonus: all

test: $(NAME)
	python3 tests/run_tests.py

valgrind: $(NAME)
	python3 tests/run_tests.py --valgrind

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all bonus test valgrind clean fclean re
