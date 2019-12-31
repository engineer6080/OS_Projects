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

static char custm[TPS_SIZE] = "-Test-";

static sem_t sem1, sem2;

void *latest_mmap_addr; // global variable to make the address returned by mmap accessible
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);

void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{
        latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
        return latest_mmap_addr;
}

/* CORRECT OUTPUT
   Thread 2 Address 0x7f6c70369000
   thread2: read OK!
   Testing NULL tps_read ok! -1
   Testing NULL tps_destroy ok -1
   Testing NULL tps_write ok -1
   Clone Address ok! 0x7f6c70369000
   Address after tps_read ok! 0x7f6c70369000
   thread1: read OK!
   After Writing, T1 has it's own page 0x7f6c70368000
   Offset read ok!
   Reading entire buffer ok!
   thread1: read OK!
 */

void *thread2(void* arg)
{
        char *buffer = malloc(TPS_SIZE);

        /* Create TPS and initialize with *msg1 */
        tps_create();
        tps_write(0, TPS_SIZE, msg1);

        char *tps_addr;

        /* Get TPS page address as allocated via mmap() */
        tps_addr = latest_mmap_addr;
        /* Cause an intentional TPS protection error */
        printf("Thread 2 Address %p\n", tps_addr);

        /* Read from TPS and make sure it contains the message */
        memset(buffer, 0, TPS_SIZE);

        //Access T2 ADDRESS
        tps_read(0, TPS_SIZE, buffer);

        assert(!memcmp(msg1, buffer, TPS_SIZE));
        printf("thread2: read OK!\n");

        /* Transfer CPU to thread 1 and get blocked */
        sem_up(sem1);
        sem_down(sem2);

        /*Writing to original copy after cloning */
        tps_write(13, (int)strlen(custm), custm); //After hello world - Testing Offset


        /* When we're back, read TPS and make sure it sill contains the original */
        memset(buffer, 0, TPS_SIZE);
        tps_read(13, (int)strlen(custm), buffer);
        int x = strcmp(buffer, "-Test-");
        assert(x == 0);
        printf("Offset read ok! \n");

        tps_read(0, TPS_SIZE, buffer);
        int y = strcmp(buffer, "Hello world!\n-Test-");
        assert(y == 0);
        printf("Reading entire buffer ok!\n");

        /* Transfer CPU to thread 1 and get blocked */
        sem_up(sem1);
        sem_down(sem2);

        /* Destroy TPS and quit */
        tps_destroy();
        return NULL;
}

void *thread1(void* arg)
{
        char *tps_addr;

        char *nullbuffer = malloc(TPS_SIZE);

        pthread_t tid;
        char *buffer = malloc(TPS_SIZE);


        /* Create thread 2 and get blocked */
        pthread_create(&tid, NULL, thread2, NULL);

        sem_down(sem1);

        tps_addr = latest_mmap_addr;

        /* Cause an intentional TPS protection error */
        //tps_addr[4096 + 1] = '\0';

        //There is no TPS, so should return -1
        int x = tps_read(0, TPS_SIZE, nullbuffer);
        assert(x == -1);
        printf("Testing NULL tps_read ok! %d\n", x);

        int y = tps_destroy();
        assert(y == -1);
        printf("Testing NULL tps_destroy ok %d\n", x);

        //Testing NULL Write
        buffer[0] = 'x';
        int z = tps_write(0, 1, buffer);
        assert(z == -1);
        printf("Testing NULL tps_write ok %d\n", z);

        /* When we're back, clone thread 2's TPS */
        tps_clone(tid);
        //Force a Read
        //tps_read(0, TPS_SIZE, nullbuffer);

        //tps_addr = latest_mmap_addr; WE CANT USE THIS BECAUSE WE ARE NOT CALLING MMAP FUNCTION inside CLONE
        printf("Clone Address ok! ");
        //Prints address of self
        tps_init(-1); //bad print function of tps thread

        /* Read the TPS and make sure it contains the original */
        memset(buffer, 0, TPS_SIZE);
        tps_read(0, TPS_SIZE, buffer);

        printf("Address after tps_read ok! %p\n",tps_addr);

        assert(!memcmp(msg1, buffer, TPS_SIZE));
        printf("thread1: read OK!\n");

        /* Modify TPS to cause a copy on write */
        buffer[0] = 'h';
        tps_write(0, 1, buffer);

        tps_addr = latest_mmap_addr;

        printf("After Writing, T1 has it's own page %p\n",tps_addr);

        /* Transfer CPU to thread 2 and get blocked */
        sem_up(sem2);
        sem_down(sem1);

        /* When we're back, make sure our modification is still there */
        memset(buffer, 0, TPS_SIZE);
        tps_read(0, TPS_SIZE, buffer);
        assert(!strcmp(msg2, buffer));
        printf("thread1: read OK!\n");

        /* Transfer CPU to thread 2 */
        sem_up(sem2);

        /* Wait for thread2 to die, and quit */
        pthread_join(tid, NULL);
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
