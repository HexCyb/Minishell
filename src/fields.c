#include <stdlib.h>
#include "minishell.h"

static int	has_unquoted_star(t_buf *field)
{
	size_t	i;

	i = 0;
	while (i < field->len)
	{
		if (field->s[i] == '*' && !field->q[i])
			return (1);
		i++;
	}
	return (0);
}

/* Appends one field, or the file names it matches when it has a '*'. */
static void	add_field(t_word **out, t_buf *field)
{
	t_word	*matches;
	t_word	**last;

	matches = NULL;
	if (has_unquoted_star(field))
		matches = glob_field(field->s, field->q);
	if (!matches)
	{
		add_word(out, buf_take(field));
		return ;
	}
	last = out;
	while (*last)
		last = &(*last)->next;
	*last = matches;
	free(buf_take(field));
}

/*
** Cuts the expanded text at blanks that came from an unquoted expansion.
** A word that had quotes but expands to nothing is still one (empty) word:
** echo "" prints an empty argument, echo $UNSET prints none.
*/
t_word	*split_fields(t_buf *b, int had_quotes)
{
	t_word	*out;
	t_buf	field;
	size_t	i;

	out = NULL;
	field = (t_buf){NULL, NULL, 0, 0};
	i = 0;
	while (i <= b->len)
	{
		if (i == b->len || (!b->q[i] && is_blank(b->s[i])))
		{
			if (field.len > 0)
				add_field(&out, &field);
		}
		else
			buf_push(&field, b->s[i], b->q[i]);
		i++;
	}
	if (!out && had_quotes)
		add_word(&out, ft_strdup(""));
	free(b->s);
	free(b->q);
	return (out);
}
