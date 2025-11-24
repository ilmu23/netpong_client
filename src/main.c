// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<main.c>>

#include <stdio.h>
#include <signal.h>
#include <kbinput/kbinput.h>

#include "menu.h"

#define handle_sig(sig)	((sigaction(sig, &action, NULL) != -1) ? 1 : 0)

static inline void	_fatal_sig(i32 sig);

static struct sigaction	action;

int	main(i32 ac, char **av) {
	if (ac < 3) {
		fprintf(stdout, "Usage: %s address port\n", PROG_NAME);
		return 1;
	}
	action.sa_handler = _fatal_sig;
	if (!handle_sig(SIGABRT) || !handle_sig(SIGALRM) || !handle_sig(SIGBUS) ||
		!handle_sig(SIGFPE) || !handle_sig(SIGHUP) || !handle_sig(SIGILL) ||
		!handle_sig(SIGINT) || !handle_sig(SIGIO) || !handle_sig(SIGPIPE) ||
		!handle_sig(SIGPROF) || !handle_sig(SIGPWR) || !handle_sig(SIGQUIT) ||
		!handle_sig(SIGSEGV) || !handle_sig(SIGSTKFLT) || !handle_sig(SIGSYS) ||
		!handle_sig(SIGTERM) || !handle_sig(SIGTRAP) || !handle_sig(SIGUSR1) ||
		!handle_sig(SIGUSR2) || !handle_sig(SIGVTALRM) ||
		!handle_sig(SIGXCPU) || !handle_sig(SIGXFSZ))
		return 1;
	return main_menu(av[1], av[2]) ? 0 : 1;
}

static inline void	_fatal_sig(i32 sig) {
	cleanup();
	action.sa_handler = SIG_DFL;
	sigaction(sig, &action, NULL);
	raise(sig);
}
