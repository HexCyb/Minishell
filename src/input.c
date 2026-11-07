#include <stdio.h>
#include <readline/readline.h>
#include <stdlib.h>
#include <unistd.h>
#include "minishell.h"

/*
** Without a terminal (a script piped in), lines are read one byte at a time:
** the commands that run may read the rest of the input themselves, exactly
** as they would under bash (try: printf 'cat\nhello\n' | ./minishell).
*/
static char	*read_raw_line(void)
{
	t_buf	b;
	char	c;
	ssize_t	n;

	b = (t_buf){NULL, NULL, 0, 0};
	n = read(STDIN_FILENO, &c, 1);
	while (n > 0 && c != '\n')
	{
		buf_push(&b, c, 1);
		n = read(STDIN_FILENO, &c, 1);
	}
	if (n <= 0 && b.len == 0)
	{
		free(b.s);
		free(b.q);
		return (NULL);
	}
	return (buf_take(&b));
}

char	*read_input(t_shell *sh, const char *prompt)
{
	if (sh->interactive)
		return (readline(prompt));
	return (read_raw_line());
}
