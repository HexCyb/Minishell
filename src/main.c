#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "minishell.h"

static void	shell_cleanup(t_shell *sh)
{
	free_tree(sh->tree);
	sh->tree = NULL;
	free(sh->line);
	sh->line = NULL;
	env_free(sh->env);
	sh->env = NULL;
	rl_clear_history();
}

/* Every process the shell forks leaves through here, memory freed. */
void	child_exit(t_shell *sh, int status)
{
	shell_cleanup(sh);
	exit(status);
}

/* Read -> tokens -> tree -> here-documents -> run. */
static void	run_line(t_shell *sh)
{
	t_token	*tokens;
	int		unclosed;

	unclosed = 0;
	tokens = lex(sh->line, &unclosed);
	if (unclosed)
	{
		print_error("syntax error: unclosed quote", NULL);
		sh->status = 2;
		return ;
	}
	sh->tree = parse(sh, tokens);
	free_tokens(tokens);
	if (!sh->tree)
		return ;
	sh->interrupted = 0;
	if (prepare_heredocs(sh, sh->tree) == 0)
		exec_node(sh, sh->tree);
	free_tree(sh->tree);
	sh->tree = NULL;
}

/* Like bash: PWD is the current directory, OLDPWD exists without a value. */
static void	init_pwd(t_shell *sh)
{
	char		*cwd;
	char		*pwd;
	struct stat	a;
	struct stat	b;

	cwd = getcwd(NULL, 0);
	pwd = env_get(sh->env, "PWD");
	if (cwd && (!pwd || stat(pwd, &a) < 0 || stat(".", &b) < 0
			|| a.st_ino != b.st_ino || a.st_dev != b.st_dev))
		env_set(&sh->env, "PWD", cwd);
	free(cwd);
	if (!env_find(sh->env, "OLDPWD"))
		env_set(&sh->env, "OLDPWD", NULL);
}

static void	shell_init(t_shell *sh, char **envp)
{
	sh->env = env_init(envp);
	init_pwd(sh);
	sh->status = 0;
	sh->interactive = isatty(STDIN_FILENO);
	sh->in_child = 0;
	sh->interrupted = 0;
	sh->should_exit = 0;
	sh->line = NULL;
	sh->tree = NULL;
	rl_outstream = stderr;
}

int	main(int argc, char **argv, char **envp)
{
	t_shell	sh;

	(void)argc;
	(void)argv;
	shell_init(&sh, envp);
	while (!sh.should_exit)
	{
		if (sh.interactive)
			signals_prompt();
		else
			signals_default();
		sh.line = read_input(&sh, PROMPT);
		if (g_signal == SIGINT)
			sh.status = 130;
		g_signal = 0;
		if (!sh.line)
			break ;
		if (sh.interactive && sh.line[0])
			add_history(sh.line);
		run_line(&sh);
		free(sh.line);
		sh.line = NULL;
	}
	if (!sh.line && !sh.should_exit && sh.interactive)
		put_str(2, "exit\n");
	shell_cleanup(&sh);
	return (sh.status);
}
