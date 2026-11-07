#include "minishell.h"

int	is_builtin(const char *name)
{
	static const char	*names[] = {"echo", "cd", "pwd", "export", "unset",
		"env", "exit", NULL};
	int					i;

	i = 0;
	while (names[i])
		if (ft_strcmp(name, names[i++]) == 0)
			return (1);
	return (0);
}

int	run_builtin(t_shell *sh, char **argv)
{
	if (ft_strcmp(argv[0], "echo") == 0)
		return (builtin_echo(argv));
	if (ft_strcmp(argv[0], "cd") == 0)
		return (builtin_cd(sh, argv));
	if (ft_strcmp(argv[0], "pwd") == 0)
		return (builtin_pwd(sh));
	if (ft_strcmp(argv[0], "export") == 0)
		return (builtin_export(sh, argv));
	if (ft_strcmp(argv[0], "unset") == 0)
		return (builtin_unset(sh, argv));
	if (ft_strcmp(argv[0], "env") == 0)
		return (builtin_env(sh, argv));
	return (builtin_exit(sh, argv));
}

/* A name: a letter or '_', then letters, digits or '_'. */
int	valid_identifier(const char *s, size_t len)
{
	size_t	i;

	if (len == 0 || !(ft_isalpha(s[0]) || s[0] == '_'))
		return (0);
	i = 1;
	while (i < len)
	{
		if (!(ft_isalnum(s[i]) || s[i] == '_'))
			return (0);
		i++;
	}
	return (1);
}
