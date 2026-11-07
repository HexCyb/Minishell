#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "minishell.h"

/* Inside a here-document only $ is special: quotes are plain text. */
char	*expand_heredoc_line(t_shell *sh, char *line)
{
	t_buf	b;
	size_t	i;

	b = (t_buf){NULL, NULL, 0, 0};
	i = 0;
	while (line[i])
	{
		if (line[i] == '$')
			i += expand_dollar(sh, line + i, &b, 1);
		else
			buf_push(&b, line[i++], 1);
	}
	free(line);
	return (buf_take(&b));
}

/*
** The text goes to a file that is unlinked at once: it stays readable through
** the descriptor, works for any size (a pipe would block past 64 KiB) and
** never stays behind.
*/
static int	open_store(int *write_fd, int *read_fd)
{
	static int	count;
	char		*num;
	char		*path;

	while (1)
	{
		num = ft_itoa(count++);
		path = ft_strjoin("/tmp/.minishell_heredoc_", num);
		free(num);
		*write_fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (*write_fd >= 0 || errno != EEXIST)
			break ;
		free(path);
	}
	if (*write_fd < 0)
	{
		print_error("here-document", strerror(errno));
		free(path);
		return (1);
	}
	*read_fd = open(path, O_RDONLY);
	unlink(path);
	free(path);
	return (0);
}

static void	eof_warning(const char *delim)
{
	char	*msg;

	msg = ft_strjoin3("warning: here-document delimited by end-of-file "
			"(wanted `", delim, "')");
	print_error(msg, NULL);
	free(msg);
}

/* Reads lines until the delimiter. 1 if Ctrl-C stopped it. */
static int	read_body(t_shell *sh, t_redir *r, int fd)
{
	char	*delim;
	char	*line;
	int		expand;

	expand = !ft_strchr(r->word, '\'') && !ft_strchr(r->word, '"');
	delim = remove_quotes(r->word);
	while (1)
	{
		line = read_input(sh, "> ");
		if (g_signal == SIGINT || !line || ft_strcmp(line, delim) == 0)
			break ;
		if (expand)
			line = expand_heredoc_line(sh, line);
		write(fd, line, ft_strlen(line));
		write(fd, "\n", 1);
		free(line);
	}
	if (!line && g_signal != SIGINT)
		eof_warning(delim);
	free(line);
	free(delim);
	return (g_signal == SIGINT);
}

static int	read_heredocs(t_shell *sh, t_redir *r)
{
	int	write_fd;
	int	stop;

	stop = 0;
	while (r && !stop)
	{
		if (r->type == T_HEREDOC)
		{
			if (open_store(&write_fd, &r->fd))
			{
				sh->status = 1;
				return (1);
			}
			if (sh->interactive)
				signals_heredoc();
			stop = read_body(sh, r, write_fd);
			close(write_fd);
			if (sh->interactive)
				signals_prompt();
		}
		r = r->next;
	}
	if (stop)
	{
		g_signal = 0;
		sh->status = 130;
	}
	return (stop);
}

/* Every here-document of the line is read first, in the order typed. */
int	prepare_heredocs(t_shell *sh, t_node *node)
{
	while (node)
	{
		if (node->kind == N_SUBSHELL && prepare_heredocs(sh, node->left))
			return (1);
		if ((node->kind == N_CMD || node->kind == N_SUBSHELL)
			&& read_heredocs(sh, node->redirs))
			return (1);
		if ((node->kind == N_PIPE || node->kind == N_AND || node->kind == N_OR)
			&& prepare_heredocs(sh, node->left))
			return (1);
		if ((node->kind == N_AND || node->kind == N_OR)
			&& prepare_heredocs(sh, node->right))
			return (1);
		node = node->next;
	}
	return (0);
}
