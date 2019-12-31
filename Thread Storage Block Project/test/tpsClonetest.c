#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>

static char msg1[TPS_SIZE] = "Hello world!\n";

/*
   This basically has a main thread create a thread1 which creates thread2
   thread 2 creats A NEW tps and then writes stuff to it.
   Thread 3 clones that thread and then destroys it
   T2 TPS should be unaffected because count > 1
   it passes it on, then T1 clones it and reads from it
   should be the same, so the assert passes!
   then we finally get back to main where it finishes
 */


void *thread3(void *id){

        //Thread passed as parameter
        pthread_t t2 = *((pthread_t*)id);
        tps_clone(t2);
        //Clone T2 and then destroy

        tps_destroy();

        return 0;
}

void *thread2(){

        pthread_t writeid;

        /* Create TPS and initialize with *msg1 */
        tps_create();
        tps_write(0, TPS_SIZE, msg1);

        /* Create cloneid and wait */
        pthread_create(&writeid, NULL, thread3, (void*)pthread_self());
        pthread_join(writeid, NULL);

        return 0;

}

void *thread1(){

        pthread_t cloneid;
        char *buffer = malloc(TPS_SIZE);

        /* Create cloneid and wait */
        pthread_create(&cloneid, NULL, thread2, NULL);
        pthread_join(cloneid, NULL);

        tps_clone(cloneid);
        tps_read(0, TPS_SIZE, buffer);

        assert(!memcmp(msg1, buffer, TPS_SIZE));

        return 0;
}


int main(int argc, char **argv)
{
        pthread_t tid;

        /* Init TPS API */
        tps_init(1);

        /* Create thread 1 and wait */
        pthread_create(&tid, NULL, thread1, NULL);
        pthread_join(tid, NULL);

        printf("Succes\n");

        return 0;
}
