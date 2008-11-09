/** \file
 * A select() test program.
 * Copyright 2008 by Michal Nazarewicz (mina86/AT/mina86.com)
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _POSIX_SOURCE 1
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>


typedef void Sigfunc(int);

static volatile sig_atomic_t got_sig_num = 0;
static void sig_handler(int signo) {
	got_sig_num = signo;
}

int main(void) {
	int fd = STDIN_FILENO, i;
	char buffer[1024];
	fd_set zbior;
	struct sigaction act, oldact[32];
	sigset_t oldsigset;

	/* Block all signals -- we will recieve them during pselect(2) only */
	sigfillset(&act.sa_mask);
	sigdelset(&act.sa_mask, SIGKILL);
	sigdelset(&act.sa_mask, SIGSTOP);
	sigprocmask(SIG_SETMASK, &act.sa_mask, &oldsigset);

	/* Now catch all signals */
	act.sa_handler = sig_handler;
	act.sa_flags = 0;
#if SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif
#if SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif
	for (i = 1; i<32; ++i) {
		sigaction(i, &act, oldact + i);
	}

	/* Do the job */
	setvbuf(stdin, NULL, _IONBF, 0);
	alarm(5);
	for(;;) {
		FD_ZERO(&zbior);
		FD_SET(fd, &zbior);

		i = pselect(fd + 1, &zbior, 0, 0, 0, &oldsigset);
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

	/* Restore old signal handlers and mask */
	for (i = 1; i<32; ++i) {
		sigaction(i, oldact + i, 0);
	}
	sigprocmask(SIG_SETMASK, &oldsigset, 0);
	return 0;
}
