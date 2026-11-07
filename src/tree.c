#include <stdlib.h>
#include <unistd.h>
#include "minishell.h"

t_node	*new_node(t_kind kind)
{
	t_node	*node;

	node = xmalloc(sizeof(t_node));
	node->kind = kind;
	node->words = NULL;
	node->redirs = NULL;
	node->left = NULL;
	node->right = NULL;
	node->next = NULL;
	return (node);
}

void	add_word(t_word **list, char *text)
{
	t_word	*word;

	word = xmalloc(sizeof(t_word));
	word->text = text;
	word->next = NULL;
	while (*list)
		list = &(*list)->next;
	*list = word;
}

void	add_redir(t_redir **list, t_tok type, char *word)
{
	t_redir	*redir;

	redir = xmalloc(sizeof(t_redir));
	redir->type = type;
	redir->word = word;
	redir->fd = -1;
	redir->next = NULL;
	while (*list)
		list = &(*list)->next;
	*list = redir;
}

static void	free_parts(t_word *word, t_redir *redir)
{
	t_word	*next_word;
	t_redir	*next_redir;

	while (word)
	{
		next_word = word->next;
		free(word->text);
		free(word);
		word = next_word;
	}
	while (redir)
	{
		next_redir = redir->next;
		if (redir->fd >= 0)
			close(redir->fd);
		free(redir->word);
		free(redir);
		redir = next_redir;
	}
}

void	free_tree(t_node *node)
{
	t_node	*next;

	while (node)
	{
		next = node->next;
		free_parts(node->words, node->redirs);
		free_tree(node->left);
		free_tree(node->right);
		free(node);
		node = next;
	}
}
