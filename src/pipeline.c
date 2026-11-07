#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "minishell.h"

/* One element of a pipeline, already in its own process. */
int	run_command(t_shell *sh, t_node *cmd)
{
	char	**argv;
	int		status;

	if (cmd->kind == N_SUBSHELL)
	{
		if (apply_redirs(sh, cmd->redirs))
			return (1);
		return (exec_node(sh, cmd->left));
	}
	argv = expand_args(sh, cmd->words);
	status = apply_redirs(sh, cmd->redirs);
	if (status == 0 && argv[0] && is_builtin(argv[0]))
		status = run_builtin(sh, argv);
	else if (status == 0 && argv[0])
		exec_external(sh, argv);
	free_strs(argv);
	return (status);
}

static void	pipeline_child(t_shell *sh, t_node *cmd, int in_fd, int *fds)
{
	sh->in_child = 1;
	signals_default();
	if (in_fd >= 0)
	{
		dup2(in_fd, STDIN_FILENO);
		close(in_fd);
	}
	if (cmd->next)
	{
		dup2(fds[1], STDOUT_FILENO);
		close(fds[1]);
		close(fds[0]);
	}
	child_exit(sh, run_command(sh, cmd));
}

static int	pipeline_failed(t_shell *sh, int in_fd, const char *what)
{
	print_error(what, strerror(errno));
	if (in_fd >= 0)
		close(in_fd);
	wait_children(sh, -1);
	return (1);
}

/*
** Every command gets its own process; each pipe connects one command's
** stdout to the next one's stdin. The shell keeps no pipe end open, so
** every reader sees end-of-file when its writer is done.
*/
int	exec_pipeline(t_shell *sh, t_node *cmd)
{
	int		fds[2];
	int		in_fd;
	pid_t	pid;

	in_fd = -1;
	signals_ignore();
	while (cmd)
	{
		if (cmd->next && pipe(fds) < 0)
			return (pipeline_failed(sh, in_fd, "pipe"));
		pid = fork();
		if (pid < 0 && cmd->next)
			close(fds[1]);
		if (pid < 0 && cmd->next)
			close(fds[0]);
		if (pid < 0)
			return (pipeline_failed(sh, in_fd, "fork"));
		if (pid == 0)
			pipeline_child(sh, cmd, in_fd, fds);
		if (in_fd >= 0)
			close(in_fd);
		in_fd = -1;
		if (cmd->next)
			close(fds[1]);
		if (cmd->next)
			in_fd = fds[0];
		cmd = cmd->next;
	}
	return (wait_children(sh, pid));
}
