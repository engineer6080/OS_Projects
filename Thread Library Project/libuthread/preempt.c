#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "preempt.h"
#include "uthread.h"

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100

/*
   You can prevent additional SIGALRM signals from
   arriving in the meantime by wrapping the critical
   part of the code with calls to sigprocmask
 */

void preempt_disable(void)
{
        /* TODO Phase 4 */
        sigset_t alarm;
        sigemptyset (&alarm);
        sigaddset (&alarm, SIGVTALRM);
/* Check if a signal has arrived; if so, reset the flag. */
        sigprocmask (SIG_BLOCK, &alarm, NULL);
}

void preempt_enable(void)
{
        /* TODO Phase 4 */
        sigset_t alarm;
        sigemptyset (&alarm);
        sigaddset (&alarm, SIGVTALRM);
        /* Check if a signal has arrived; if so, reset the flag. */
        sigprocmask (SIG_UNBLOCK, &alarm, NULL);
}


//SIGNAL HANDLER
void catch_stop(int sig)
{
        uthread_yield();

}


//Setup Alarm and timer
void preempt_start(void)
{
        /* TODO Phase 4 */
        //fprintf(stderr, "HELLO\n");

        /* Initialize the data structures for the interval timer. */
        struct sigaction setup_action;

        //sigset_t block_mask;

        //sigaddset (&block_mask, SIGINT);
        //sigfillset (&setup_action.block_mask);
        setup_action.sa_handler = catch_stop;

        if(sigemptyset (&setup_action.sa_mask) ==-1) {
                printf("ERR\n");
        }

        setup_action.sa_flags = 0;

        //setup_action.sa_mask = block_mask;
        sigaction (SIGVTALRM, &setup_action, NULL);

        //Block other terminal-generated signals while handler runs.
        //sigaddset (&block_mask, SIGQUIT);


        //Configure a timer which will fire an alarm
        //100 times per second
        struct itimerval it;

        //represents the number of whole seconds of elapsed time
        it.it_interval.tv_sec = 0;
        it.it_interval.tv_usec = 10000;
        it.it_value.tv_sec = 0;
        it.it_value.tv_usec = 10000;
        //it.it_value = it.it_interval;

        /* Install the timer and get the context we can manipulate. */
        setitimer (ITIMER_VIRTUAL, &it, NULL);
        //fprintf(stderr, "END\n");




}
