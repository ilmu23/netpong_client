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
#include <stdlib.h>
#include <string.h>
#include <kbinput/kbinput.h>

#include "menu.h"

#define handle_sig(sig)	((sigaction(sig, &action, NULL) != -1) ? 1 : 0)

static inline u8	_check_args(const char **av);

static inline void	_fatal_sig(i32 sig);

static struct sigaction	action;

int	main(i32 ac, char **av) {
	if (ac < 3) {
		fprintf(stdout, "Usage: %s address port\n", PROG_NAME);
		return 1;
	}
	if (!_check_args((const char **)&av[1]))
		return 1;
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

static inline u8	_check_args(const char **av) {
	const char	*end;
	const char	*cur;
	size_t		i;
	i64			n;

	for (i = 0, cur = av[0]; i < 4; i++, cur = (const char *)(uintptr_t)end + 1) {
		n = strtol(cur, (char **)&end, 10);
		if (n > UINT8_MAX || cur == end || *end != ((i < 3) ? '.' : '\0')) {
			fprintf(stderr, "Invalid address: %s\n", av[0]);
			return 0;
		}
	}
	n = strtol(av[1], (char **)&end, 10);
	if (n > UINT16_MAX || av[1] == end || *end != '\0') {
		fprintf(stderr, "Invalid port: %s\n", av[1]);
		return 0;
	}
	return 1;
}

static inline void	_fatal_sig(i32 sig) {
	cleanup();
	action.sa_handler = SIG_DFL;
	sigaction(sig, &action, NULL);
	raise(sig);
}
