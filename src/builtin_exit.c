#include <stdlib.h>
#include "minishell.h"

/* Like bash: names that are not valid identifiers are silently ignored. */
int	builtin_unset(t_shell *sh, char **argv)
{
	int	i;

	i = 1;
	while (argv[i])
	{
		if (valid_identifier(argv[i], ft_strlen(argv[i])))
			env_unset(&sh->env, argv[i]);
		i++;
	}
	return (0);
}

/* A whole number that fits in a long, blanks around it allowed.
** out: the number modulo 256, as the process will see it. */
static int	parse_status(const char *s, unsigned char *out)
{
	unsigned long	n;
	int				neg;
	int				digits;

	while (*s == ' ' || *s == '\t')
		s++;
	neg = (*s == '-');
	if (*s == '-' || *s == '+')
		s++;
	n = 0;
	digits = 0;
	while (ft_isdigit(*s))
	{
		n = n * 10 + (*s++ - '0');
		if (++digits > 19 || n > (unsigned long)__LONG_MAX__ + neg)
			return (0);
	}
	while (*s == ' ' || *s == '\t')
		s++;
	if (!digits || *s)
		return (0);
	*out = (unsigned char)n;
	if (neg)
		*out = (unsigned char)(0 - n);
	return (1);
}

/*
** exit [n]: n % 256 (exit -1 -> 255). Not a number: status 2.
** Too many arguments: an error, and the shell goes on.
*/
int	builtin_exit(t_shell *sh, char **argv)
{
	unsigned char	n;

	if (sh->interactive && !sh->in_child)
		put_str(2, "exit\n");
	if (!argv[1])
	{
		sh->should_exit = 1;
		return (sh->status);
	}
	if (!parse_status(argv[1], &n))
	{
		print_error3("exit", argv[1], "numeric argument required");
		sh->should_exit = 1;
		return (2);
	}
	if (argv[2])
	{
		print_error("exit", "too many arguments");
		return (1);
	}
	sh->should_exit = 1;
	return (n);
}
