/** \file
 * A select() test program.
 * $Id: select-test.c,v 1.1 2007/12/31 15:38:50 mina86 Exp $
 */

#define _POSIX_SOURCE  1

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>


typedef void Sigfunc(int);

static volatile sig_atomic_t got_sig_num = 0;
static void sig_handler(int);
static Sigfunc *mysignal(int, Sigfunc*);

int main(void) {
	int fd = STDIN_FILENO, i;
	char buffer[1024];
	fd_set zbior;

	setvbuf(stdin, NULL, _IONBF, 0);

	for (i = 1; i<28; ++i) {
		mysignal(i, sig_handler);
	}

	alarm(5);
	for(;;) {
		FD_ZERO(&zbior);
		FD_SET(fd, &zbior);

		i = select(fd + 1, &zbior, 0, 0, 0);
		if (i > 0) {
			fgets(buffer, sizeof buffer, stdin);
			printf("input: %s", buffer);
		} else if (i == 0) {
			/* dead code -- there is no timeout */
			fputs("select: returned 0\n", stderr);
			return 1;
		} else if (errno != EINTR) {
			perror("select");
			return 1;
		} else if (got_sig_num == SIGALRM) {
			alarm(5);
			puts("tick");
		} else {
			printf("signal: %d\n", (int)got_sig_num);
			if (got_sig_num == 15) break;
		}
	}

	return 0;
}


static void sig_handler(int signo) {
	got_sig_num = signo;
}

static Sigfunc* mysignal(int signo, Sigfunc *func) {
	struct sigaction act, oact;

	act.sa_handler = func;
	sigfillset(&act.sa_mask);         /* wypelnienie zbioru sygnalow */
	sigdelset(&act.sa_mask, SIGKILL); /* wyrzucenie sigkill i sigstop */
	sigdelset(&act.sa_mask, SIGSTOP);

	act.sa_flags = 0;
#if SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif
#if SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	if (sigaction(signo, &act, &oact) < 0) {
		return SIG_ERR;
	}
	return oact.sa_handler;
}

