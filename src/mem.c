#include <stdlib.h>
#include <unistd.h>
#include "minishell.h"

/* Running out of memory is not something a shell can recover from. */
void	*xmalloc(size_t size)
{
	void	*p;

	p = malloc(size);
	if (!p)
	{
		write(2, "minishell: out of memory\n", 25);
		exit(1);
	}
	return (p);
}

void	free_strs(char **strs)
{
	size_t	i;

	if (!strs)
		return ;
	i = 0;
	while (strs[i])
		free(strs[i++]);
	free(strs);
}

int	ft_isalpha(int c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int	ft_isdigit(int c)
{
	return (c >= '0' && c <= '9');
}

int	ft_isalnum(int c)
{
	return (ft_isalpha(c) || ft_isdigit(c));
}
