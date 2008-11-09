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

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

typedef void Sigfunc(int);

void sig_handler(int);
void sig_alarm(int);
Sigfunc *signal_(int, Sigfunc*);

struct timeval czas;

int main(void) {
	fd_set zbior;
	int fd = STDIN_FILENO;
	char buffer[1024];
	int i;

	setvbuf(stdin, NULL, _IONBF, 0);

	FD_ZERO(&zbior);
	FD_SET(fd, &zbior);
	/*
	 * blokowanie wszystkich sygnalow (to moze byc nieprzenosne,
	 * ale chyba bez sensu je po nazwach wszystkie po kolei przerabiac ;) )
	 */
	for(i=1; i<28; ++i) {
		signal_(i, sig_handler);
	}
	/* ustawienie innego sygnalu na alarm */
	signal_(SIGALRM, sig_alarm);

	alarm(5);
	while(1) {
		i = select(STDIN_FILENO+1, &zbior, NULL, NULL, NULL);
		if (i > 0) {
			fgets(buffer, sizeof buffer, stdin);
			printf("input: %s", buffer);
		} else if (i == 0) {
			fputs("select: returned 0\n", stderr);
			break;
		} else if (errno != EINTR) {
			perror("select");
			break;
		} else {
			/* do nothing */
		}
	}
	return 1;
}

void sig_handler(int signo) {
	printf("received %d\n", signo);
}

/****************************************************************
 * to jest zmodyfikowana funkcja signal
 * ktora jest po to, zeby w czasie obslugi
 * jakiegos sygnalu, inne byly blokowane
 * po zakonczeniu funkcji obslugi, stara maska
 * jest przywracana i sygnaly beda obsluzone
 * ponadto select zostanie powtorzony po obsludze sygnalu
 ***************************************************************/
Sigfunc* signal_(int signo, Sigfunc *func) {
	struct sigaction act, oact;

	act.sa_handler = func;
	sigfillset(&act.sa_mask); /* wypelnienie zbioru sygnalow */
	sigdelset(&act.sa_mask, SIGKILL); /* wyrzucenie z niego sigkill i sigstop */
	sigdelset(&act.sa_mask, SIGSTOP);
	act.sa_flags = 0;
#ifdef SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;
#endif
#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif
	if (sigaction(signo, &act, &oact) < 0) {
		return(SIG_ERR);
	}
	return(oact.sa_handler);
}

void sig_alarm(int signo) {
	(void)signo;
	printf("tick\n");
	alarm(5);
}
