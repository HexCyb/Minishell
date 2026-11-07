#include <stdlib.h>
#include "minishell.h"

t_env	*env_find(t_env *env, const char *key)
{
	while (env && ft_strcmp(env->key, key) != 0)
		env = env->next;
	return (env);
}

/* NULL when the variable does not exist or has no value (export NAME) */
char	*env_get(t_env *env, const char *key)
{
	t_env	*var;

	var = env_find(env, key);
	if (!var)
		return (NULL);
	return (var->value);
}

/* value == NULL keeps an existing value: "export NAME" never erases it */
void	env_set(t_env **env, const char *key, const char *value)
{
	t_env	*var;
	t_env	**last;

	var = env_find(*env, key);
	if (var)
	{
		if (!value)
			return ;
		free(var->value);
		var->value = ft_strdup(value);
		return ;
	}
	var = xmalloc(sizeof(t_env));
	var->key = ft_strdup(key);
	var->value = NULL;
	if (value)
		var->value = ft_strdup(value);
	var->next = NULL;
	last = env;
	while (*last)
		last = &(*last)->next;
	*last = var;
}

void	env_unset(t_env **env, const char *key)
{
	t_env	*var;

	while (*env)
	{
		if (ft_strcmp((*env)->key, key) == 0)
		{
			var = *env;
			*env = var->next;
			free(var->key);
			free(var->value);
			free(var);
			return ;
		}
		env = &(*env)->next;
	}
}

/* The environment the shell starts with, one level deeper (SHLVL). */
t_env	*env_init(char **envp)
{
	t_env	*env;
	char	*eq;
	char	*key;
	char	*level;
	long	n;

	env = NULL;
	while (envp && *envp)
	{
		eq = ft_strchr(*envp, '=');
		if (eq)
		{
			key = ft_strndup(*envp, eq - *envp);
			env_set(&env, key, eq + 1);
			free(key);
		}
		envp++;
	}
	n = 0;
	level = env_get(env, "SHLVL");
	while (level && ft_isdigit(*level) && n < 1000)
		n = n * 10 + (*level++ - '0');
	level = ft_itoa(n + 1);
	env_set(&env, "SHLVL", level);
	free(level);
	return (env);
}
