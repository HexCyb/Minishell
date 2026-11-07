#include <stdlib.h>
#include "minishell.h"

/*
** s points at a '$'. Appends what it stands for and returns how many
** characters of s were used:
**   $?      the status of the last command
**   $NAME   the variable's value (nothing if it is not set)
**   $0      "minishell", $1 .. $9: nothing
**   $"..." or $'...' outside quotes: the '$' disappears, like in bash
**   anything else: a plain '$'
** Text from an unquoted expansion is marked unquoted: it may be split into
** several words and its '*' may match file names.
*/
size_t	expand_dollar(t_shell *sh, const char *s, t_buf *b, int in_quotes)
{
	size_t	n;
	char	*text;

	if (s[1] == '?')
	{
		text = ft_itoa(sh->status);
		buf_push_str(b, text, in_quotes);
		free(text);
		return (2);
	}
	if (ft_isalpha(s[1]) || s[1] == '_')
	{
		n = 1;
		while (ft_isalnum(s[n]) || s[n] == '_')
			n++;
		text = ft_strndup(s + 1, n - 1);
		buf_push_str(b, env_get(sh->env, text), in_quotes);
		free(text);
		return (n);
	}
	if (ft_isdigit(s[1]) && s[1] == '0')
		buf_push_str(b, "minishell", in_quotes);
	if (ft_isdigit(s[1]))
		return (2);
	if (!in_quotes && (s[1] == '\'' || s[1] == '"'))
		return (1);
	buf_push(b, '$', 1);
	return (1);
}
