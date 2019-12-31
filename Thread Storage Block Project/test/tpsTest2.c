#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tps.h>
#include <sem.h>

static char msg1[TPS_SIZE] = "Hello world!\n";
static char msg2[TPS_SIZE] = "hello world!\n";

static sem_t sem1, sem2;

void *latest_mmap_addr; // global variable to make the address returned by mmap accessible

void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
        latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
        return latest_mmap_addr;
}

/* CORRECT OUTPUT
   thread2: read OK!
   thread1: read OK!
   thread1: read OK!
   TPS protection error!
   Segmentation fault (core dumped)
 */

void *thread2(void* arg)
{
        char *buffer = malloc(TPS_SIZE);

        /* Create TPS and initialize with *msg1 */
        tps_create();
        tps_write(0, TPS_SIZE, msg1);

        /* Read from TPS and make sure it contains the message */
        memset(buffer, 0, TPS_SIZE);

        tps_read(0, TPS_SIZE, buffer);

        assert(!memcmp(msg1, buffer, TPS_SIZE));
        printf("thread2: read OK!\n");

        /* Transfer CPU to thread 1 and get blocked */
        sem_up(sem1);
        sem_down(sem2);

        /* When we're back, read TPS and make sure it sill contains the original */
        /* Destroy TPS and quit */
        tps_destroy();

        return NULL;
}

//T1 Clones T2, but T2 Deletes itself (before copy on write)
//Comes back to T1, we make sure that the data is still there
//We are able to create a new page on write as well
void *thread1(void* arg)
{
        pthread_t tid;
        char *buffer = malloc(TPS_SIZE);
        char *tps_addr = malloc(TPS_SIZE);

        /* Create thread 2 and get blocked */
        pthread_create(&tid, NULL, thread2, NULL);

        sem_down(sem1);

        /* When we're back, clone thread 2's TPS */
        tps_clone(tid);

        /* Transfer CPU to thread 2 and get blocked */
        sem_up(sem2);
        //sem_down(sem1);

        /* Wait for thread2 to die, and quit */
        pthread_join(tid, NULL);

        /* Read the TPS and make sure it contains the original */
        memset(buffer, 0, TPS_SIZE);
        tps_read(0, TPS_SIZE, buffer);

        //UPPER CASE HELLO WORLD
        assert(!memcmp(msg1, buffer, TPS_SIZE));
        printf("thread1: read OK!\n");

        /* Modify TPS to cause a copy on write */
        buffer[0] = 'h';
        tps_write(0, 1, buffer);

        //Lower case version
        tps_read(0, TPS_SIZE, buffer);

        assert(!memcmp(msg2, buffer, TPS_SIZE));
        printf("thread1: read OK!\n");

        /* Get TPS page address as allocated via mmap() */
        tps_addr = latest_mmap_addr;

        /* Cause an intentional TPS protection error */
        tps_addr[0] = '\0';

        tps_destroy();

        return NULL;
}

int main(int argc, char **argv)
{
        pthread_t tid;

        /* Create two semaphores for thread synchro */
        sem1 = sem_create(0);
        sem2 = sem_create(0);

        /* Init TPS API */
        tps_init(1);

        /* Create thread 1 and wait */
        pthread_create(&tid, NULL, thread1, NULL);
        pthread_join(tid, NULL);

        /* Destroy resources and quit */
        sem_destroy(sem1);
        sem_destroy(sem2);
        return 0;
}
