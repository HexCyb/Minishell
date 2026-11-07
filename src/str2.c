#include "minishell.h"

int	ft_strcmp(const char *a, const char *b)
{
	size_t	i;

	i = 0;
	while (a[i] && a[i] == b[i])
		i++;
	return ((unsigned char)a[i] - (unsigned char)b[i]);
}

int	ft_strncmp(const char *a, const char *b, size_t n)
{
	size_t	i;

	i = 0;
	while (i < n && a[i] && a[i] == b[i])
		i++;
	if (i == n)
		return (0);
	return ((unsigned char)a[i] - (unsigned char)b[i]);
}

char	*ft_strchr(const char *s, int c)
{
	while (*s)
	{
		if (*s == (char)c)
			return ((char *)s);
		s++;
	}
	if ((char)c == '\0')
		return ((char *)s);
	return (NULL);
}

char	*ft_itoa(long n)
{
	char			digits[24];
	int				i;
	unsigned long	u;

	i = 23;
	digits[i] = '\0';
	u = n;
	if (n < 0)
		u = -(unsigned long)n;
	if (u == 0)
		digits[--i] = '0';
	while (u > 0)
	{
		digits[--i] = '0' + u % 10;
		u /= 10;
	}
	if (n < 0)
		digits[--i] = '-';
	return (ft_strdup(digits + i));
}
