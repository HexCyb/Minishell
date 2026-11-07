#include <stdlib.h>
#include <unistd.h>
#include "minishell.h"

/* declare -x NAME="value", with " \ $ ` escaped as bash does */
static void	print_var(t_env *var)
{
	t_buf		b;
	const char	*v;

	b = (t_buf){NULL, NULL, 0, 0};
	buf_push_str(&b, "declare -x ", 1);
	buf_push_str(&b, var->key, 1);
	if (var->value)
	{
		buf_push_str(&b, "=\"", 1);
		v = var->value;
		while (*v)
		{
			if (*v == '"' || *v == '\\' || *v == '$' || *v == '`')
				buf_push(&b, '\\', 1);
			buf_push(&b, *v++, 1);
		}
		buf_push(&b, '"', 1);
	}
	buf_push(&b, '\n', 1);
	write(STDOUT_FILENO, b.s, b.len);
	free(b.s);
	free(b.q);
}

/* Every variable once, in byte order of the names. */
static void	print_sorted(t_env *env)
{
	t_env	*var;
	t_env	*best;
	char	*last;

	last = NULL;
	while (1)
	{
		best = NULL;
		var = env;
		while (var)
		{
			if ((!last || ft_strcmp(var->key, last) > 0)
				&& (!best || ft_strcmp(var->key, best->key) < 0))
				best = var;
			var = var->next;
		}
		if (!best)
			return ;
		print_var(best);
		last = best->key;
	}
}

/* NAME, NAME=value or NAME+=value */
static int	export_one(t_shell *sh, const char *arg)
{
	const char	*eq;
	size_t		len;
	char		*key;
	char		*value;

	eq = ft_strchr(arg, '=');
	len = ft_strlen(arg);
	if (eq)
		len = eq - arg - (eq > arg && eq[-1] == '+');
	if (!valid_identifier(arg, len))
	{
		key = ft_strjoin3("`", arg, "'");
		print_error3("export", key, "not a valid identifier");
		free(key);
		return (1);
	}
	key = ft_strndup(arg, len);
	if (!eq)
		env_set(&sh->env, key, NULL);
	else if (eq[-1] == '+' && env_get(sh->env, key))
	{
		value = ft_strjoin(env_get(sh->env, key), eq + 1);
		env_set(&sh->env, key, value);
		free(value);
	}
	else
		env_set(&sh->env, key, eq + 1);
	free(key);
	return (0);
}

int	builtin_export(t_shell *sh, char **argv)
{
	int	status;
	int	i;

	if (!argv[1])
	{
		print_sorted(sh->env);
		return (0);
	}
	status = 0;
	i = 1;
	while (argv[i])
		if (export_one(sh, argv[i++]))
			status = 1;
	return (status);
}
