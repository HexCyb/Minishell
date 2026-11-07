#include <stdlib.h>
#include "minishell.h"

/*
** One word as typed -> zero, one or more words:
** 1. $ expansions (not inside single quotes)
** 2. quote removal (each character remembers if it was quoted)
** 3. unquoted expansion results are split on blanks
** 4. an unquoted '*' matches file names in the current directory
*/
t_word	*expand_word(t_shell *sh, const char *raw)
{
	t_buf	b;
	char	quote;
	int		had_quotes;
	size_t	i;

	b = (t_buf){NULL, NULL, 0, 0};
	quote = 0;
	had_quotes = 0;
	i = 0;
	while (raw[i])
	{
		if (!quote && (raw[i] == '\'' || raw[i] == '"'))
		{
			quote = raw[i++];
			had_quotes = 1;
		}
		else if (quote && raw[i] == quote && ++i)
			quote = 0;
		else if (raw[i] == '$' && quote != '\'')
			i += expand_dollar(sh, raw + i, &b, quote == '"');
		else
			buf_push(&b, raw[i++], quote != 0);
	}
	return (split_fields(&b, had_quotes));
}

/* All the words of a command, expanded, as an argv array. */
char	**expand_args(t_shell *sh, t_word *words)
{
	t_word	*fields;
	t_word	*all;
	t_word	*next;
	char	**argv;
	size_t	n;

	all = NULL;
	n = 0;
	while (words)
	{
		fields = expand_word(sh, words->text);
		while (fields)
		{
			next = fields->next;
			fields->next = NULL;
			add_word(&all, fields->text);
			free(fields);
			fields = next;
			n++;
		}
		words = words->next;
	}
	argv = xmalloc((n + 1) * sizeof(char *));
	n = 0;
	while (all)
	{
		argv[n++] = all->text;
		next = all->next;
		free(all);
		all = next;
	}
	argv[n] = NULL;
	return (argv);
}

static void	free_words(t_word *w)
{
	t_word	*next;

	while (w)
	{
		next = w->next;
		free(w->text);
		free(w);
		w = next;
	}
}

/* A file name must expand to exactly one word, else it is ambiguous. */
int	expand_redir_word(t_shell *sh, const char *raw, char **out)
{
	t_word	*fields;

	fields = expand_word(sh, raw);
	if (!fields || fields->next)
	{
		print_error(raw, "ambiguous redirect");
		free_words(fields);
		return (1);
	}
	*out = fields->text;
	free(fields);
	return (0);
}

/* The quotes of a here-document delimiter are removed, nothing is expanded. */
char	*remove_quotes(const char *raw)
{
	t_buf	b;
	char	quote;

	b = (t_buf){NULL, NULL, 0, 0};
	quote = 0;
	while (*raw)
	{
		if (!quote && (*raw == '\'' || *raw == '"'))
			quote = *raw;
		else if (quote && *raw == quote)
			quote = 0;
		else
			buf_push(&b, *raw, 1);
		raw++;
	}
	return (buf_take(&b));
}
