#include <stdlib.h>
#include "minishell.h"

/*
** list      := pipeline { ("&&" | "||") pipeline }
** pipeline  := command { "|" command }
** command   := "(" list ")" { redirection } | { word | redirection }+
*/
typedef struct s_parser
{
	t_token	*tok;
	int		error;
}	t_parser;

static t_node	*parse_list(t_parser *p);

/* Only the first error is reported, like bash. */
static void	syntax_error(t_parser *p)
{
	char	*msg;

	if (p->error)
		return ;
	p->error = 1;
	if (p->tok->type == T_END)
		msg = ft_strdup("syntax error near unexpected token `newline'");
	else
		msg = ft_strjoin3("syntax error near unexpected token `",
				p->tok->text, "'");
	print_error(msg, NULL);
	free(msg);
}

static int	is_redir(t_tok type)
{
	return (type == T_IN || type == T_OUT || type == T_APPEND
		|| type == T_HEREDOC);
}

static void	parse_redir(t_parser *p, t_redir **list)
{
	t_tok	type;

	type = p->tok->type;
	p->tok = p->tok->next;
	if (p->tok->type != T_WORD)
		return (syntax_error(p));
	add_redir(list, type, ft_strdup(p->tok->text));
	p->tok = p->tok->next;
}

/* A simple command, or a list in parentheses; both may carry redirections. */
static t_node	*parse_command(t_parser *p)
{
	t_node	*node;

	node = new_node(N_CMD);
	if (p->tok->type == T_LPAREN)
	{
		node->kind = N_SUBSHELL;
		p->tok = p->tok->next;
		node->left = parse_list(p);
		if (!p->error && p->tok->type != T_RPAREN)
			syntax_error(p);
		if (!p->error)
			p->tok = p->tok->next;
	}
	while (!p->error && (is_redir(p->tok->type)
			|| (node->kind == N_CMD && p->tok->type == T_WORD)))
	{
		if (p->tok->type != T_WORD)
			parse_redir(p, &node->redirs);
		else
		{
			add_word(&node->words, ft_strdup(p->tok->text));
			p->tok = p->tok->next;
		}
	}
	if (!p->error && ((node->kind == N_CMD && !node->words && !node->redirs)
			|| p->tok->type == T_WORD || p->tok->type == T_LPAREN))
		syntax_error(p);
	return (node);
}

static t_node	*parse_pipeline(t_parser *p)
{
	t_node	*first;
	t_node	*last;
	t_node	*pipe;

	first = parse_command(p);
	if (p->error || p->tok->type != T_PIPE)
		return (first);
	pipe = new_node(N_PIPE);
	pipe->left = first;
	last = first;
	while (!p->error && p->tok->type == T_PIPE)
	{
		p->tok = p->tok->next;
		last->next = parse_command(p);
		last = last->next;
	}
	return (pipe);
}

static t_node	*parse_list(t_parser *p)
{
	t_node	*left;
	t_node	*op;

	left = parse_pipeline(p);
	while (!p->error && (p->tok->type == T_AND || p->tok->type == T_OR))
	{
		op = new_node(N_OR);
		if (p->tok->type == T_AND)
			op->kind = N_AND;
		p->tok = p->tok->next;
		op->left = left;
		op->right = parse_pipeline(p);
		left = op;
	}
	return (left);
}

/* The syntax tree of a line, or NULL (empty line or syntax error, status 2) */
t_node	*parse(t_shell *sh, t_token *tokens)
{
	t_parser	p;
	t_node		*tree;

	if (tokens->type == T_END)
		return (NULL);
	p.tok = tokens;
	p.error = 0;
	tree = parse_list(&p);
	if (!p.error && p.tok->type != T_END)
		syntax_error(&p);
	if (!p.error)
		return (tree);
	free_tree(tree);
	sh->status = 2;
	return (NULL);
}
