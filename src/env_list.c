#include <stdlib.h>
#include "minishell.h"

/* KEY=value for every variable that has a value, for execve() */
char	**env_to_array(t_env *env)
{
	char	**out;
	t_env	*var;
	size_t	n;

	n = 0;
	var = env;
	while (var)
	{
		n += (var->value != NULL);
		var = var->next;
	}
	out = xmalloc((n + 1) * sizeof(char *));
	n = 0;
	while (env)
	{
		if (env->value)
			out[n++] = ft_strjoin3(env->key, "=", env->value);
		env = env->next;
	}
	out[n] = NULL;
	return (out);
}

void	env_free(t_env *env)
{
	t_env	*next;

	while (env)
	{
		next = env->next;
		free(env->key);
		free(env->value);
		free(env);
		env = next;
	}
}
