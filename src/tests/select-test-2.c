/** \file
 * A select() test program.
 * $Id: select-test-2.c,v 1.1 2007/12/31 15:38:50 mina86 Exp $
 */

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

int main() {
	fd_set zbior;
	int fd = STDIN_FILENO;
	struct sigaction sigact;
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
	//ustawienie innego sygnalu na alarm
	signal_(SIGALRM, sig_alarm);

	alarm(5);
	while(1) {
		i = select(STDIN_FILENO+1, &zbior, NULL, NULL, NULL);
		if( i > 0 ) {
			fgets(buffer, sizeof buffer, stdin);
			printf("input: %s", buffer);
		}
		if ( i < 0 ) continue; //bez tego fgets sie blokuje po sygnale
		else {

		}
	}
	return 0;
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
	sigfillset(&act.sa_mask); //wypelnienie zbioru sygnalow
	sigdelset(&act.sa_mask, SIGKILL); //wyrzucenie z niego sigkill i sigstop
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
	printf("tick\n");
	alarm(5);
}