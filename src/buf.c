#include <stdlib.h>
#include "minishell.h"

static void	buf_grow(t_buf *b)
{
	char	*s;
	char	*q;
	size_t	i;

	b->cap = b->cap * 2 + 32;
	s = xmalloc(b->cap + 1);
	q = xmalloc(b->cap + 1);
	i = 0;
	while (i < b->len)
	{
		s[i] = b->s[i];
		q[i] = b->q[i];
		i++;
	}
	free(b->s);
	free(b->q);
	b->s = s;
	b->q = q;
}

/* quoted: 1 if the character must stay as it is (no splitting, no globbing) */
void	buf_push(t_buf *b, char c, char quoted)
{
	if (b->len == b->cap)
		buf_grow(b);
	b->s[b->len] = c;
	b->q[b->len] = quoted;
	b->len++;
	b->s[b->len] = '\0';
	b->q[b->len] = '\0';
}

void	buf_push_str(t_buf *b, const char *s, char quoted)
{
	while (s && *s)
		buf_push(b, *s++, quoted);
}

/* The text as a new string; the buffer is emptied. */
char	*buf_take(t_buf *b)
{
	char	*s;

	if (!b->s)
		s = ft_strdup("");
	else
		s = ft_strndup(b->s, b->len);
	free(b->s);
	free(b->q);
	b->s = NULL;
	b->q = NULL;
	b->len = 0;
	b->cap = 0;
	return (s);
}
