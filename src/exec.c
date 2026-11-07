#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "minishell.h"

/* A builtin (or a command made only of redirections) runs in the shell
** itself, so cd and export change the shell. Its redirections are undone. */
static int	run_in_shell(t_shell *sh, t_node *cmd, char **argv)
{
	int	saved_in;
	int	saved_out;
	int	status;

	saved_in = dup(STDIN_FILENO);
	saved_out = dup(STDOUT_FILENO);
	status = apply_redirs(sh, cmd->redirs);
	if (status == 0 && argv[0])
		status = run_builtin(sh, argv);
	dup2(saved_in, STDIN_FILENO);
	dup2(saved_out, STDOUT_FILENO);
	close(saved_in);
	close(saved_out);
	return (status);
}

static int	exec_simple(t_shell *sh, t_node *cmd)
{
	char	**argv;
	int		status;
	pid_t	pid;

	argv = expand_args(sh, cmd->words);
	if (!argv[0] || is_builtin(argv[0]))
	{
		status = run_in_shell(sh, cmd, argv);
		free_strs(argv);
		return (status);
	}
	signals_ignore();
	pid = fork();
	if (pid == 0)
	{
		sh->in_child = 1;
		signals_default();
		if (apply_redirs(sh, cmd->redirs))
		{
			free_strs(argv);
			child_exit(sh, 1);
		}
		exec_external(sh, argv);
	}
	free_strs(argv);
	if (pid < 0)
	{
		print_error("fork", strerror(errno));
		return (1);
	}
	return (wait_children(sh, pid));
}

/* ( list ) runs in a child process: nothing it changes reaches the shell. */
static int	exec_subshell(t_shell *sh, t_node *node)
{
	pid_t	pid;

	signals_ignore();
	pid = fork();
	if (pid < 0)
	{
		print_error("fork", strerror(errno));
		return (1);
	}
	if (pid == 0)
	{
		sh->in_child = 1;
		signals_default();
		if (apply_redirs(sh, node->redirs))
			child_exit(sh, 1);
		child_exit(sh, exec_node(sh, node->left));
	}
	return (wait_children(sh, pid));
}

/* The status of every step is $? for the next one: false || echo $? -> 1 */
int	exec_node(t_shell *sh, t_node *node)
{
	int	status;

	if (node->kind == N_AND || node->kind == N_OR)
	{
		status = exec_node(sh, node->left);
		if (sh->interrupted || sh->should_exit)
			return (status);
		if ((node->kind == N_AND) == (status == 0))
			status = exec_node(sh, node->right);
		return (status);
	}
	if (node->kind == N_PIPE)
		status = exec_pipeline(sh, node->left);
	else if (node->kind == N_SUBSHELL)
		status = exec_subshell(sh, node);
	else
		status = exec_simple(sh, node);
	sh->status = status;
	return (status);
}
