#include <stdio.h>
#include <readline/readline.h>
#include <unistd.h>
#include "minishell.h"

volatile sig_atomic_t	g_signal = 0;

/* Ctrl-C at the prompt: forget the line, start a fresh one. */
static void	on_prompt_sigint(int sig)
{
	g_signal = sig;
	write(2, "\n", 1);
	rl_on_new_line();
	rl_replace_line("", 0);
	rl_redisplay();
}

/* Ctrl-C in a here-document: readline returns at once and the line stops. */
static void	on_heredoc_sigint(int sig)
{
	g_signal = sig;
	rl_done = 1;
}

static int	no_event(void)
{
	return (0);
}

static void	set_handler(int sig, void (*handler)(int), int flags)
{
	struct sigaction	sa;

	sa.sa_handler = handler;
	sa.sa_flags = flags;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, NULL);
}

void	signals_prompt(void)
{
	rl_event_hook = NULL;
	set_handler(SIGINT, on_prompt_sigint, SA_RESTART);
	set_handler(SIGQUIT, SIG_IGN, 0);
}

/* With an event hook set, readline checks rl_done while it waits for keys. */
void	signals_heredoc(void)
{
	rl_event_hook = no_event;
	set_handler(SIGINT, on_heredoc_sigint, 0);
	set_handler(SIGQUIT, SIG_IGN, 0);
}

/* The shell waits for its children: Ctrl-C and Ctrl-\ are theirs. */
void	signals_ignore(void)
{
	set_handler(SIGINT, SIG_IGN, 0);
	set_handler(SIGQUIT, SIG_IGN, 0);
}

void	signals_default(void)
{
	set_handler(SIGINT, SIG_DFL, 0);
	set_handler(SIGQUIT, SIG_DFL, 0);
}
