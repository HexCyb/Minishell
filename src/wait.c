#include <errno.h>
#include <sys/wait.h>
#include "minishell.h"

static const char	*signal_name(int sig)
{
	if (sig == SIGTERM)
		return ("Terminated");
	if (sig == SIGKILL)
		return ("Killed");
	if (sig == SIGQUIT)
		return ("Quit");
	if (sig == SIGSEGV)
		return ("Segmentation fault");
	if (sig == SIGABRT)
		return ("Aborted");
	if (sig == SIGBUS)
		return ("Bus error");
	if (sig == SIGFPE)
		return ("Floating point exception");
	if (sig == SIGHUP)
		return ("Hangup");
	if (sig == SIGALRM)
		return ("Alarm clock");
	if (sig == SIGILL)
		return ("Illegal instruction");
	if (sig == SIGUSR1)
		return ("User defined signal 1");
	if (sig == SIGUSR2)
		return ("User defined signal 2");
	return ("Killed by a signal");
}

/*
** Like bash: when the last command is killed by a signal, the shell says so
** ("Terminated", "Quit (core dumped)"...), except for Ctrl-C (only a new line
** at the prompt) and a broken pipe. Its status is 128 + the signal.
*/
static int	killed(t_shell *sh, int wstatus)
{
	int	sig;

	sig = WTERMSIG(wstatus);
	if (sig == SIGINT)
		sh->interrupted = 1;
	if (sig == SIGINT && sh->interactive && !sh->in_child)
		put_str(2, "\n");
	if (sig == SIGINT || sig == SIGPIPE)
		return (128 + sig);
	put_str(2, signal_name(sig));
	if (WCOREDUMP(wstatus))
		put_str(2, " (core dumped)");
	put_str(2, "\n");
	return (128 + sig);
}

/* Waits for every child. The status is the last command's. */
int	wait_children(t_shell *sh, pid_t last)
{
	int		wstatus;
	int		status;
	pid_t	pid;

	status = 1;
	while (1)
	{
		pid = wait(&wstatus);
		if (pid < 0 && errno == EINTR)
			continue ;
		if (pid < 0)
			break ;
		if (pid == last && WIFSIGNALED(wstatus))
			status = killed(sh, wstatus);
		else if (pid == last)
			status = WEXITSTATUS(wstatus);
	}
	return (status);
}
