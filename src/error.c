#include <stdlib.h>
#include <unistd.h>
#include "minishell.h"

int	is_blank(int c)
{
	return (c == ' ' || c == '\t' || c == '\n');
}

void	put_str(int fd, const char *s)
{
	write(fd, s, ft_strlen(s));
}

static char	*append_part(char *msg, const char *part)
{
	char	*joined;

	if (!part)
		return (msg);
	joined = ft_strjoin3(msg, ": ", part);
	free(msg);
	return (joined);
}

/* "minishell: a: b: c" written at once, so lines from two processes never mix */
void	print_error3(const char *a, const char *b, const char *c)
{
	char	*msg;
	char	*line;

	msg = ft_strjoin("minishell: ", a);
	msg = append_part(msg, b);
	msg = append_part(msg, c);
	line = ft_strjoin(msg, "\n");
	put_str(2, line);
	free(msg);
	free(line);
}

void	print_error(const char *who, const char *what)
{
	print_error3(who, what, NULL);
}
