#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "minishell.h"

/*
** Runs a program in the current (child) process. Never returns:
** 127 if it cannot be found, 126 if it is found but cannot be run.
*/
void	exec_external(t_shell *sh, char **argv)
{
	char	*path;
	char	**envp;
	int		code;

	path = find_command(sh, argv[0], &code);
	if (path)
	{
		envp = env_to_array(sh->env);
		execve(path, argv, envp);
		code = 126;
		if (errno == ENOENT)
			code = 127;
		print_error(argv[0], strerror(errno));
		free_strs(envp);
		free(path);
	}
	free_strs(argv);
	child_exit(sh, code);
}
