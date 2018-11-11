/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2015.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation, either version 3 or (at your option) any      *
* later version. This program is distributed without any warranty.  See   *
* the file COPYING.gpl-v3 for details.                                    *
\*************************************************************************/

/* Listing 23-1 */

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include "tlpi_hdr.h"

// schedule library
#include <sched.h>

static volatile sig_atomic_t gotAlarm = 0;
                        /* Set nonzero on receipt of SIGALRM */

/* Retrieve and display the real time, and (if 'includeTimer' is
   TRUE) the current value and interval for the ITIMER_REAL timer */

static void
displayTimes(const char *msg, Boolean includeTimer)
{
    struct itimerval itv;
    static struct timeval start;
    struct timeval curr;
    static int callNum = 0;             /* Number of calls to this function */

    if (callNum == 0)                   /* Initialize elapsed time meter */
        if (gettimeofday(&start, NULL) == -1)
            errExit("gettimeofday");

    if (callNum % 20 == 0)              /* Print header every 20 lines */
        printf("       Elapsed   Value Interval\n");

    if (gettimeofday(&curr, NULL) == -1)
        errExit("gettimeofday");
    printf("%-7s %6.2f", msg, curr.tv_sec - start.tv_sec +
                        (curr.tv_usec - start.tv_usec) / 1000000.0);

    if (includeTimer) {
        if (getitimer(ITIMER_REAL, &itv) == -1)
            errExit("getitimer");
        printf("  %6.2f  %6.2f",
                itv.it_value.tv_sec + itv.it_value.tv_usec / 1000000.0,
                itv.it_interval.tv_sec + itv.it_interval.tv_usec / 1000000.0);
    }

    printf("\n");
    callNum++;
}

static void
sigalrmHandler(int sig)
{
    gotAlarm = 1;
}
// exemplo de entrada de argumentos ./real_timer 1 0 2 0 r 10 0
// primeiro arg e segundo (1 0) diz quando o alarm começa
// terceiro arg e quarto (2 0) diz de quanto em quanto tempo o intervalo dispara
// quinto arg (r) é o policy de escalonamento podem ser {r, f, o}
// sexto arg é a prioridade 1 a 99
// e o último argumento é o pid do processo que será alterado a pridade (0 diz que é o processo que está chamando)
int
main(int argc, char *argv[])
{
	
	// inicio programa de escalonamento
	int j, pol;
    struct sched_param sp;

    if (argc < 3 || strchr("rfobi", argv[5][0]) == NULL)
        usageErr("%s policy priority [pid...]\n"
                "    policy is 'r' (RR), 'f' (FIFO), "
	#ifdef SCHED_BATCH              /* Linux-specific */
                "'b' (BATCH), "
	#endif
	#ifdef SCHED_IDLE               /* Linux-specific */
                "'i' (IDLE), "
	#endif
                "or 'o' (OTHER)\n",
                argv[0]);

    pol = (argv[5][0] == 'r') ? SCHED_RR :
                (argv[5][0] == 'f') ? SCHED_FIFO :
	#ifdef SCHED_BATCH
                (argv[5][0] == 'b') ? SCHED_BATCH :
	#endif
	#ifdef SCHED_IDLE
                (argv[5][0] == 'i') ? SCHED_IDLE :
	#endif
                SCHED_OTHER;

    sp.sched_priority = getInt(argv[6], 0, "priority");
    for (j = 7; j < argc; j++)
        if (sched_setscheduler(getLong(argv[j], 0, "pid"), pol, &sp) == -1)
            errExit("sched_setscheduler");
	
	// FIM PROGRAMA ESCALONAMENTO
	
    int pid = getpid();
    printf("pid %d\n", pid);
	
    struct itimerval itv;
    clock_t prevClock;
    struct sigaction sa;

    if (argc > 1 && strcmp(argv[1], "--help") == 0)
        usageErr("%s [secs [usecs [int-secs [int-usecs]]]]\n", argv[0]);

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigalrmHandler;
    if (sigaction(SIGALRM, &sa, NULL) == -1)
        errExit("sigaction");

    /* Set timer from the command-line arguments */

    itv.it_value.tv_sec = (argc > 1) ? getLong(argv[1], 0, "secs") : 2;
    itv.it_value.tv_usec = (argc > 2) ? getLong(argv[2], 0, "usecs") : 0;
    itv.it_interval.tv_sec = (argc > 3) ? getLong(argv[3], 0, "int-secs") : 0;
    itv.it_interval.tv_usec = (argc > 4) ? getLong(argv[4], 0, "int-usecs") : 0;

    displayTimes("START:", FALSE);
    if (setitimer(ITIMER_REAL, &itv, NULL) == -1)
        errExit("setitimer");

    prevClock = clock();

    for (;;) {

        /* Inner loop consumes at least 0.5 seconds CPU time */

        while (((clock() - prevClock) * 10 / CLOCKS_PER_SEC) < 5) {
            if (gotAlarm) {                     /* Did we get a signal? */
                gotAlarm = 0;
                displayTimes("ALARM:", TRUE);
            }
        }
        
        pause();

        prevClock = clock();
        displayTimes("Main: ", TRUE);
    }
}
