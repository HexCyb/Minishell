#include <stdlib.h>
#include <unistd.h>
#include "minishell.h"

/* -n, -nnn: no newline. Options stop at the first other word. */
static int	is_n_option(const char *s)
{
	if (*s++ != '-' || *s != 'n')
		return (0);
	while (*s == 'n')
		s++;
	return (*s == '\0');
}

/* The whole output in one write(). */
int	builtin_echo(char **argv)
{
	t_buf	b;
	int		newline;
	size_t	i;

	b = (t_buf){NULL, NULL, 0, 0};
	newline = 1;
	i = 1;
	while (argv[i] && is_n_option(argv[i]))
	{
		newline = 0;
		i++;
	}
	while (argv[i])
	{
		buf_push_str(&b, argv[i], 1);
		if (argv[++i])
			buf_push(&b, ' ', 1);
	}
	if (newline)
		buf_push(&b, '\n', 1);
	write(STDOUT_FILENO, b.s, b.len);
	free(b.s);
	free(b.q);
	return (0);
}

int	builtin_pwd(t_shell *sh)
{
	char	*cwd;

	cwd = getcwd(NULL, 0);
	if (!cwd && env_get(sh->env, "PWD"))
		cwd = ft_strdup(env_get(sh->env, "PWD"));
	if (!cwd)
	{
		print_error("pwd", "cannot get the current directory");
		return (1);
	}
	put_str(STDOUT_FILENO, cwd);
	put_str(STDOUT_FILENO, "\n");
	free(cwd);
	return (0);
}

int	builtin_env(t_shell *sh, char **argv)
{
	t_env	*var;
	char	*line;

	if (argv[1])
	{
		print_error("env", "no options or arguments are supported");
		return (1);
	}
	var = sh->env;
	while (var)
	{
		if (var->value)
		{
			line = ft_strjoin3(var->key, "=", var->value);
			put_str(STDOUT_FILENO, line);
			put_str(STDOUT_FILENO, "\n");
			free(line);
		}
		var = var->next;
	}
	return (0);
}
