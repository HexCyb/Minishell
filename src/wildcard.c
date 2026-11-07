#include <dirent.h>
#include <stdlib.h>
#include "minishell.h"

/* '*' matches any run of characters, but only when it was not quoted. */
static int	match(const char *name, const char *p, const char *q)
{
	while (*p)
	{
		if (*p == '*' && !*q)
		{
			while (*p == '*' && !*q)
			{
				p++;
				q++;
			}
			if (!*p)
				return (1);
			while (*name)
				if (match(name++, p, q))
					return (1);
			return (0);
		}
		if (*name != *p)
			return (0);
		name++;
		p++;
		q++;
	}
	return (*name == '\0');
}

/* Insertion into a list kept in byte order, like bash with LC_ALL=C. */
static void	insert_sorted(t_word **list, char *name)
{
	t_word	*word;

	while (*list && ft_strcmp((*list)->text, name) < 0)
		list = &(*list)->next;
	word = xmalloc(sizeof(t_word));
	word->text = name;
	word->next = *list;
	*list = word;
}

/*
** The names in the current directory that match the pattern, sorted.
** Hidden names only match a pattern that starts with a '.', and "." and ".."
** never match. NULL if nothing matches: the word is then kept as it is.
*/
t_word	*glob_field(const char *s, const char *q)
{
	DIR				*dir;
	struct dirent	*entry;
	t_word			*matches;

	dir = opendir(".");
	if (!dir)
		return (NULL);
	matches = NULL;
	entry = readdir(dir);
	while (entry)
	{
		if ((entry->d_name[0] != '.' || s[0] == '.')
			&& ft_strcmp(entry->d_name, ".") && ft_strcmp(entry->d_name, "..")
			&& match(entry->d_name, s, q))
			insert_sorted(&matches, ft_strdup(entry->d_name));
		entry = readdir(dir);
	}
	closedir(dir);
	return (matches);
}
