#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "minishell.h"

static int	open_target(t_tok type, const char *file)
{
	if (type == T_IN)
		return (open(file, O_RDONLY));
	if (type == T_OUT)
		return (open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644));
	return (open(file, O_WRONLY | O_CREAT | O_APPEND, 0644));
}

static int	redirect_one(t_shell *sh, t_redir *r)
{
	char	*file;
	int		fd;
	int		target;

	if (r->type == T_HEREDOC)
		return (dup2(r->fd, STDIN_FILENO) < 0);
	if (expand_redir_word(sh, r->word, &file))
		return (1);
	fd = open_target(r->type, file);
	if (fd < 0)
	{
		print_error(file, strerror(errno));
		free(file);
		return (1);
	}
	free(file);
	target = STDOUT_FILENO;
	if (r->type == T_IN)
		target = STDIN_FILENO;
	dup2(fd, target);
	close(fd);
	return (0);
}

/* Left to right, like bash: the first failure stops the command (status 1). */
int	apply_redirs(t_shell *sh, t_redir *r)
{
	while (r)
	{
		if (redirect_one(sh, r))
			return (1);
		r = r->next;
	}
	return (0);
}
