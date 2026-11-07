#include <stdlib.h>
#include "minishell.h"

/* Longest operator first: "||" before "|", "<<" before "<". */
static size_t	operator_at(const char *s, t_tok *type)
{
	static const char	*ops[] = {"||", "&&", "<<", ">>", "|", "<", ">",
		"(", ")", NULL};
	static const t_tok	types[] = {T_OR, T_AND, T_HEREDOC, T_APPEND, T_PIPE,
		T_IN, T_OUT, T_LPAREN, T_RPAREN};
	size_t				i;

	i = 0;
	while (ops[i])
	{
		if (ft_strncmp(s, ops[i], ft_strlen(ops[i])) == 0)
		{
			*type = types[i];
			return (ft_strlen(ops[i]));
		}
		i++;
	}
	return (0);
}

/* A word ends at a blank or an operator outside quotes. -1: unclosed quote. */
static long	word_len(const char *s)
{
	long	i;
	char	quote;
	t_tok	unused;

	i = 0;
	while (s[i] && !is_blank(s[i]) && !operator_at(s + i, &unused))
	{
		if (s[i] == '\'' || s[i] == '"')
		{
			quote = s[i++];
			while (s[i] && s[i] != quote)
				i++;
			if (!s[i])
				return (-1);
		}
		i++;
	}
	return (i);
}

static void	push_token(t_token **tail, t_tok type, char *text)
{
	t_token	*tok;

	tok = xmalloc(sizeof(t_token));
	tok->type = type;
	tok->text = text;
	tok->next = NULL;
	(*tail)->next = tok;
	*tail = tok;
}

void	free_tokens(t_token *tok)
{
	t_token	*next;

	while (tok)
	{
		next = tok->next;
		free(tok->text);
		free(tok);
		tok = next;
	}
}

/* The line as a list of tokens ending with T_END. NULL if a quote is open. */
t_token	*lex(const char *line, int *unclosed)
{
	t_token	head;
	t_token	*tail;
	t_tok	type;
	long	len;

	head.next = NULL;
	tail = &head;
	while (*line)
	{
		if (is_blank(*line) && line++)
			continue ;
		len = operator_at(line, &type);
		if (len == 0)
		{
			type = T_WORD;
			len = word_len(line);
		}
		if (len < 0)
		{
			*unclosed = 1;
			free_tokens(head.next);
			return (NULL);
		}
		push_token(&tail, type, ft_strndup(line, len));
		line += len;
	}
	push_token(&tail, T_END, NULL);
	return (head.next);
}
