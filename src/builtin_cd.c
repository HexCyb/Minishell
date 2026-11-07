#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "minishell.h"

/* cd, cd -, cd dir. OLDPWD and PWD follow, like in bash. */
static const char	*target_dir(t_shell *sh, char **argv)
{
	const char	*dir;

	if (!argv[1] || ft_strcmp(argv[1], "--") == 0)
	{
		dir = env_get(sh->env, "HOME");
		if (!dir)
			print_error("cd", "HOME not set");
		return (dir);
	}
	if (ft_strcmp(argv[1], "-") == 0)
	{
		dir = env_get(sh->env, "OLDPWD");
		if (!dir)
			print_error("cd", "OLDPWD not set");
		return (dir);
	}
	return (argv[1]);
}

static void	update_pwd(t_shell *sh, char *old, const char *dir)
{
	char	*cwd;

	cwd = getcwd(NULL, 0);
	if (!cwd && old)
		cwd = ft_strjoin3(old, "/", dir);
	if (old)
		env_set(&sh->env, "OLDPWD", old);
	if (cwd)
		env_set(&sh->env, "PWD", cwd);
	free(cwd);
}

int	builtin_cd(t_shell *sh, char **argv)
{
	const char	*dir;
	char		*old;

	if (argv[1] && argv[2])
	{
		print_error("cd", "too many arguments");
		return (1);
	}
	dir = target_dir(sh, argv);
	if (!dir)
		return (1);
	if (dir[0] == '\0')
		return (0);
	old = getcwd(NULL, 0);
	if (!old && env_get(sh->env, "PWD"))
		old = ft_strdup(env_get(sh->env, "PWD"));
	if (chdir(dir) < 0)
	{
		print_error3("cd", dir, strerror(errno));
		free(old);
		return (1);
	}
	if (argv[1] && ft_strcmp(argv[1], "-") == 0)
	{
		put_str(STDOUT_FILENO, dir);
		put_str(STDOUT_FILENO, "\n");
	}
	update_pwd(sh, old, dir);
	free(old);
	return (0);
}
