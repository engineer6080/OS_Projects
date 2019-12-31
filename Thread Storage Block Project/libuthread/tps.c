#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

/* TODO: Phase 2 */

//PAGE
typedef struct page {
        //number of TPS sharing the same page
        int count; //reference counter
        void *address;

}page;

//Storage block for each thread
typedef struct storage {
        pthread_t TID; // Thread ID
        page *pageP;

}storage;



queue_t Thread_TPS;
int findA(void *data, void *arg);
int PrintAddr();

static void segv_handler(int sig, siginfo_t *si, void *context)
{
        /*
         * Get the address corresponding to the beginning of the page where the
         * fault occurred
         */
        void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

        /*
         * Iterate through all the TPS areas and find if p_fault matches one of them
         */
        storage * temp;

        queue_iterate(Thread_TPS,findA,(void **)p_fault,(void **)&temp);

        if(temp) {
                //if (/* There is a match */)
                /* Printf the following error message */
                fprintf(stderr, "TPS protection error!\n");
        }

        /* In any case, restore the default signal handlers */
        signal(SIGSEGV, SIG_DFL);
        signal(SIGBUS, SIG_DFL);
        /* And transmit the signal again in order to cause the program to crash */
        raise(sig);
}

int firsttime = 0;

int tps_init(int segv)
{
        /* TODO: Phase 2 */

        if (segv > 0) {
                struct sigaction sa;

                sigemptyset(&sa.sa_mask);
                sa.sa_flags = SA_SIGINFO;
                sa.sa_sigaction = segv_handler;
                sigaction(SIGBUS, &sa, NULL);
                sigaction(SIGSEGV, &sa, NULL);
        }
        else if(segv == -1) { ///magic print function
                PrintAddr();

        }

        //This should hold all the thread storage areas
        if(firsttime == 0) { //TEmporary
                Thread_TPS = queue_create();
                firsttime = 1;
        }
        else{
                //Already been initialized
                return -1;
        }

        return 0;

}


int findT(void *data, void *arg){

        storage *tmp = data;

        if(tmp->TID == (pthread_t)(intptr_t)arg) {
                //fprintf(stderr, "%d - %d\n", (tmp->TID), (uthread_t)(intptr_t)arg);
                return 1;

        }

        return 0;

}

int findA(void *data, void *arg){

        storage *tmp = data;

        if(tmp->pageP->address == arg) {
                //fprintf(stderr, "%d - %d\n", (tmp->TID), (uthread_t)(intptr_t)arg);
                return 1;

        }

        return 0;

}

//Find and Print Address of Self
int PrintAddr(){

        storage * tmp;
        queue_iterate(Thread_TPS,findT,(void **)pthread_self(),(void **)&tmp);

        if(tmp) {
                printf("%p\n", tmp->pageP->address);
                return 0;
        }

        //Not found
        return -1;


}

//Create TPS for this particular thread
int tps_create(void)
{
        /* TODO: Phase 2 */
        storage * tpsB;

        //Check if the thread already has a TPS
        if(queue_iterate(Thread_TPS,findT,(void **)pthread_self(),NULL)) {
                return -1;
        }

        enter_critical_section();

        tpsB = (storage*)malloc(sizeof(storage));
        //Allocate memory for page
        tpsB->pageP = (page *)malloc(sizeof(page));

        tpsB->pageP->address = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        assert(tpsB->pageP->address!= MAP_FAILED);

        tpsB->pageP->count = 1; //REFERENCE COUNT

        //Associate thread ID with current thread
        tpsB->TID = pthread_self();
        queue_enqueue(Thread_TPS,tpsB);

        exit_critical_section();

        return 0;

}


int tps_destroy(void)
{
        /* TODO: Phase 2 */

        storage *tmp = NULL;

        //Check if the thread already has a TPS
        queue_iterate(Thread_TPS,findT,(void **)pthread_self(), (void **)&tmp);

        //It means the thread does not have a TPS
        if(tmp == NULL) {

                return -1;
        }
        else{ //There is a TPS for it
              //Check if something else is pointing to it
                if(tmp->pageP->count == 1) {
                        munmap(tmp->pageP->address, TPS_SIZE); //Delete the memory map
                        queue_delete(Thread_TPS,tmp); //Rmove from Queue
                        free(tmp->pageP); //free pointer
                        free(tmp);
                        return 0;
                }
                tmp->pageP->count -= 1;
                queue_delete(Thread_TPS,tmp); //Rmove from Queue
                //pending
                free(tmp); //free data
        }

        return 0;

}

int tps_read(size_t offset, size_t length, char *buffer)
{
        /* TODO: Phase 2 */

        storage *tmp = NULL;

        //Check if the thread already has a TPS
        queue_iterate(Thread_TPS,findT,(void **)pthread_self(), (void **)&tmp);


        //If the Thread has a TPS
        if(tmp != NULL) {
                enter_critical_section();

                //Give Read Permission
                mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_READ);

                memcpy(buffer,(void *)((tmp->pageP->address+ offset)), length);

                //Block after done
                mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_NONE);

                exit_critical_section();
        }
        else{   //There is no TPS for this thread

                return -1;
        }

        return 0;
}

//Go through make sure its not its own and then create a new page for it
//THIS IS HIGHLY WASTEFUL
int findCpy(void *data, void *arg){
        storage *tmp = data;
        //We have a match with something in queue that points to same address
        //The matching address with count 1 is the original owner
        if(tmp->pageP->address == arg && tmp->pageP->count != 1) { //caller becomes owner
                //printf("before %p\n", arg);
                enter_critical_section();

                mprotect((void*) (arg), TPS_SIZE, PROT_READ | PROT_WRITE);
                mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_READ | PROT_WRITE);

                tmp->pageP = malloc(sizeof(page));
                //Create new PAge for it
                tmp->pageP->address = mmap(NULL, TPS_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                memcpy((void*)tmp->pageP->address,(void*)arg, TPS_SIZE);

                mprotect((void*) (arg), TPS_SIZE, PROT_NONE);
                mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_NONE);

                //printf("after %p\n", tmp->pageP->address);
                tmp->pageP->count = 1;

                exit_critical_section();
        }
        return 0;
}

int tps_write(size_t offset, size_t length, char *buffer)
{
        storage *tmp = NULL;

        //Check if the thread already has a TPS
        queue_iterate(Thread_TPS,findT,(void **)pthread_self(), (void **)&tmp);

        //If it has a TPS
        if(tmp != NULL) {

                //This means that we need to make a new page
                //Also parent could write and the count > 1
                if(tmp->pageP->count > 1) {
                        //Go through every single object to find address associated with this page
                        //Create a new page for it
                        //Copy current state of page. THIS IS HIGHLY WASTEFUL
                        //queue_iterate(Thread_TPS,findCpy,(void **)tmp->pageP->address, NULL);

                        page *newP; //new page

                        //Allocate memory
                        newP = malloc(sizeof(page)); //Aready done in clone and create

                        //Create new TPS for it. Memory
                        newP->address = mmap(NULL, TPS_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                        newP->count = 1;

                        //Just make a new page for the calling thread
                        enter_critical_section();

                        mprotect((void*) (newP->address), TPS_SIZE, PROT_READ | PROT_WRITE);
                        mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_READ | PROT_WRITE);

                        //copy over from the pointer it had to it's new page
                        memcpy((void*)newP->address,(void*)tmp->pageP->address, TPS_SIZE);

                        mprotect((void*) (newP->address), TPS_SIZE, PROT_NONE);
                        mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_NONE);

                        //We preserve the original page that all the clones were pointing to
                        //Decrementing here will decrement for all of them

                        //Personal note..
                        //In this line, the count is shared across all the ones that share this page
                        //We can decrement and this will affect all
                        tmp->pageP->count -= 1;
                        //UPDATE NEW PAGE. Now it has a new count 1, because it is it's own page
                        tmp->pageP = newP;


                        exit_critical_section();

                }

                //Make change on current page(new independent one if count  was > 1)
                enter_critical_section();
                //TEmporary give Write access
                mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_WRITE);

                memcpy((void*)((tmp->pageP->address)+offset),buffer, length);

                //Block Write access
                mprotect((void*) (tmp->pageP->address), TPS_SIZE, PROT_NONE);

                exit_critical_section();

        }
        else{   //There is no TPS for this thread

                return -1;
        }

        return 0;
}

int tps_clone(pthread_t tid)
{
        /* TODO: Phase 2 */
        storage *CloneFrom = NULL;

        storage *CloneTo = NULL;

        //enter_critical_section();

        //The address we are cloning from
        queue_iterate(Thread_TPS,findT,(void **)tid, (void **)&CloneFrom);

        //This is who we are cloning to
        queue_iterate(Thread_TPS,findT,(void **)pthread_self(), (void **)&CloneTo);


        //if the calling thread doesn't have a TPS
        if(CloneTo == NULL) {
                //Create a TPS for it
                //tps_create();

                enter_critical_section();
                storage *tpsB;

                tpsB = (storage*)malloc(sizeof(storage));
                //Allocate memory for page
                tpsB->pageP = malloc(sizeof(page));

                //SAVING SPACE
                //tpsB->pageP->address = mmap(NULL, TPS_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

                tpsB->pageP->count = 1; //REFERENCE COUNT

                //Associate thread ID with current thread
                tpsB->TID = pthread_self();

                queue_enqueue(Thread_TPS,tpsB);

                exit_critical_section();

                //Queue it back into CloneTo
                queue_iterate(Thread_TPS,findT,(void **)pthread_self(), (void **)&CloneTo);
        }


        //Check that both of them are not NULL
        if(CloneFrom != NULL && CloneTo != NULL) {

                enter_critical_section();

                //mprotect((void*) (CloneTo->pageP->address), TPS_SIZE, PROT_READ | PROT_WRITE);
                //mprotect((void*) (CloneFrom->pageP->address), TPS_SIZE, PROT_READ | PROT_WRITE);

                //NAIVE WAY
                //memcpy((void*)CloneTo->pageP->address,(void*)CloneFrom->pageP->address, TPS_SIZE);
                //NEW WAY
                CloneTo->pageP = CloneFrom->pageP;

                //Shouldn't matter because the pageP is shared
                //CloneFrom->pageP->count += 1;
                //Increment count of page
                CloneTo->pageP->count += 1; //They both point to the same count

                //mprotect((void*) (CloneTo->pageP->address), TPS_SIZE, PROT_NONE);
                //mprotect((void*) (CloneFrom->pageP->address), TPS_SIZE, PROT_NONE);

                exit_critical_section();

        }
        else{   //There is no TPS for one of the threads

                //exit_critical_section();
                return -1;
        }

        //exit_critical_section();
        return 0;
}
