#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

typedef struct semaphore {
        /* TODO: Phase 1 */

        size_t count; //number of times the resource can be shared
        queue_t blocked; //Queue of threads waiting to use resource
        //pthread_mutex_t lock; //mutex context

}semaphore;



sem_t sem_create(size_t count)
{
        /* TODO: Phase 1 */

        //Create and intialize sempahore
        semaphore *sem;
        sem = (semaphore*)malloc(sizeof(semaphore));
        sem->count = count;
        sem->blocked =  queue_create(); //init block queue

        return sem;
}

int sem_destroy(sem_t sem)
{
        /* TODO: Phase 1 */
        if(!sem) {
                return -1;
        }

        free(sem);
        //Don't need the queue anymore
        queue_destroy(sem->blocked);

        return 0;
}


//Just a test print function
int printT(void *data, void *arg){
        //printf("%s\n", );

        pthread_t *tmp = data;
        printf("hey %lu\n",(unsigned long)tmp);

        return 0;

}

//Grab a resource
int sem_down(sem_t sem)
{
        /* TODO: Phase 1 */

        if(!sem) { //If semaphore is NULL
                return -1;
        }

        //spinlock_acquire(sem->lock);
        enter_critical_section();
        if(sem->count == 0) { //waiting for resouce to become available
                /* Block self */;
                pthread_t x = pthread_self();
                //printf("hey %lu\n",(unsigned long)pthread_self());
                //queue_iterate(blocked, printT, NULL, NULL);
                queue_enqueue(sem->blocked,(void *)x);

                thread_block(); //block current active thread

        }
        sem->count -= 1;
        //spinlock_release(sem->lock);
        exit_critical_section();

        return 0;
}


//Free resource
int sem_up(sem_t sem)
{
        /* TODO: Phase 1 */

        if(!sem) { //If semaphore is NULL
                return -1;
        }
        void *x = NULL;

        //spinlock_acquire(sem->lock);
        enter_critical_section();
        sem->count += 1;
        /* Wake up first in line */
        queue_dequeue(sem->blocked,(void **)&x);

        if(x) { //check if exists
                //just need to pass in TID to unblock
                thread_unblock((intptr_t)x);

        }
        //spinlock_release(sem->lock);
        exit_critical_section();

        return 0;

}
