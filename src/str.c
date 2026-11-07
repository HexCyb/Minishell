#include "minishell.h"

size_t	ft_strlen(const char *s)
{
	size_t	len;

	len = 0;
	while (s && s[len])
		len++;
	return (len);
}

char	*ft_strndup(const char *s, size_t n)
{
	char	*copy;
	size_t	i;

	copy = xmalloc(n + 1);
	i = 0;
	while (i < n && s[i])
	{
		copy[i] = s[i];
		i++;
	}
	copy[i] = '\0';
	return (copy);
}

char	*ft_strdup(const char *s)
{
	return (ft_strndup(s, ft_strlen(s)));
}

char	*ft_strjoin(const char *a, const char *b)
{
	return (ft_strjoin3(a, b, ""));
}

char	*ft_strjoin3(const char *a, const char *b, const char *c)
{
	size_t	la;
	size_t	lb;
	size_t	lc;
	char	*out;
	size_t	i;

	la = ft_strlen(a);
	lb = ft_strlen(b);
	lc = ft_strlen(c);
	out = xmalloc(la + lb + lc + 1);
	i = 0;
	while (i < la + lb + lc)
	{
		if (i < la)
			out[i] = a[i];
		else if (i < la + lb)
			out[i] = b[i - la];
		else
			out[i] = c[i - la - lb];
		i++;
	}
	out[i] = '\0';
	return (out);
}
