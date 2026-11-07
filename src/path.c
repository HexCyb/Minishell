#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "minishell.h"

/* A name with a '/' (or any name when PATH is empty) is used as a path. */
static char	*check_path(const char *path, int *code)
{
	struct stat	st;

	if (stat(path, &st) < 0)
	{
		print_error(path, strerror(errno));
		*code = 126;
		if (errno == ENOENT)
			*code = 127;
		return (NULL);
	}
	*code = 126;
	if (S_ISDIR(st.st_mode))
		print_error(path, "Is a directory");
	else if (access(path, X_OK) < 0)
		print_error(path, strerror(errno));
	else
		return (ft_strdup(path));
	return (NULL);
}

/* dir/name if it is an executable file. denied: found, but not executable */
static char	*try_dir(const char *dir, size_t len, const char *name, int *denied)
{
	char		*base;
	char		*path;
	struct stat	st;

	if (len == 0)
		base = ft_strdup(".");
	else
		base = ft_strndup(dir, len);
	path = ft_strjoin3(base, "/", name);
	free(base);
	if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode))
	{
		if (access(path, X_OK) == 0)
			return (path);
		*denied = 1;
	}
	free(path);
	return (NULL);
}

/* The first executable PATH entry; an empty entry means the current dir. */
char	*find_command(t_shell *sh, char *name, int *code)
{
	const char	*dirs;
	const char	*end;
	char		*path;
	int			denied;

	dirs = env_get(sh->env, "PATH");
	if (name[0] && (ft_strchr(name, '/') || !dirs || !dirs[0]))
		return (check_path(name, code));
	denied = 0;
	while (name[0] && dirs)
	{
		end = ft_strchr(dirs, ':');
		if (!end)
			end = dirs + ft_strlen(dirs);
		path = try_dir(dirs, end - dirs, name, &denied);
		if (path)
			return (path);
		dirs = NULL;
		if (*end)
			dirs = end + 1;
	}
	*code = 127;
	if (denied)
		*code = 126;
	if (denied)
		print_error(name, "Permission denied");
	else
		print_error(name, "command not found");
	return (NULL);
}
