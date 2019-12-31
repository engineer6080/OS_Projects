#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "context.h"
#include "preempt.h"
#include "queue.h"
#include "uthread.h"

//To track TIDs
int current_tid=0;

//States
queue_t ready_queue; //ready to run
queue_t zombie_queue; //process that die/exit
queue_t blocked_queue; //parents that need to be blocked

//Status of Thread
#define READY 1
#define RUNNING 2
#define BLOCKED 3
#define ZOMBIE 4

typedef struct TCB {
        uthread_t TID;
        int status;
        int* stack_ptr; //pointer to stack
        int retval;
        int child_tid;
        ucontext_t *cntxt; //context

} TCB;


//Current Active Thread
TCB *activeT;

//Find and return queue item thread which has a matching TID
int find_thread(void *data, void *arg);


/*
   Yield active thread and switch context to next thread in ready_queue
 */

void uthread_yield(void)
{

        //SENSITIVE DATA
        preempt_disable();

        //Get pointer to current active thread
        TCB* temp = activeT;


        //ActiveT is the only one in Running at the moment

        //Check if active
        if(temp->status == READY) {
                //ENQUEUE Current ACTIVE Thread
                queue_enqueue(ready_queue, temp);
        }

        //Dequeue next element into activeT. Could be the only element in Q
        queue_dequeue(ready_queue, (void **)&activeT);
        //        return;
        //}

        preempt_enable();

        //Switch context to next Thread ready to run
        uthread_ctx_switch((temp->cntxt), (activeT->cntxt));


}

//Return TID of the Active Thread
uthread_t uthread_self(void)
{
        return activeT->TID;

}

//This initializes the Main Thread.(FIRST TIME ONLY)
void initmainthread(){

        //Main thread
        TCB *mainT;

        //we init the main queues for the states of process
        //mainT = malloc(sizeof(TCB)); //main thread ALWAYS running
        mainT = (TCB*)malloc(sizeof(TCB));

        mainT->TID = current_tid; //Main Thread TID = 0
        mainT->status = READY; //initial status
        mainT->stack_ptr = uthread_ctx_alloc_stack(); ///pointer to top of stack
        mainT->cntxt = malloc(sizeof(uthread_ctx_t)); //to save context
        mainT->child_tid = -1;

        //init queues for the different statuses
        ready_queue = queue_create();
        zombie_queue = queue_create();
        blocked_queue = queue_create();

        //initially active thread is main thread
        activeT = mainT;

        //Should be called when the uthread library is initializating and sets up preemption
        preempt_start();

}


/*
   Create, intialize and return TID of new Thread
   Put in Ready Queue

 */
int uthread_create(uthread_func_t func, void *arg)
{

        //when called first time we must first initialize the uthread library
        //by registering the so-far execution  flow of the applicaiton as the main thread
        //that the library can later schedule for exeuction like any thread



        //BEING CALLED FIRST TIME
        //you should put this initialization code in a separate function for clarity.
        if(current_tid == 0) {
                initmainthread();
                current_tid++;
        }

        //Creating new thread BLOCK
        TCB *newT = malloc(sizeof(TCB));
        newT->TID = current_tid;
        newT->status = READY;
        newT->stack_ptr = uthread_ctx_alloc_stack(); //stack pointer
        newT->cntxt = malloc(sizeof(uthread_ctx_t));
        newT->child_tid = -1;

        current_tid++;


        int err = uthread_ctx_init(newT->cntxt, newT->stack_ptr, func, arg);

        //INIT DATA STRUCT
        preempt_disable();

        //enqueue in ready_queue
        queue_enqueue(ready_queue, newT);

        // if initialized correct thread then return tid of the thread else -1
        if(err==-1) {
                return -1;
        }


        //RE-ENABLE preempt
        preempt_enable();

        //Return TID
        return newT->TID;

}

int find_thread(void *data, void *arg){

        TCB *tmp = data;

        if(tmp->TID == (uthread_t)(intptr_t)arg) {
                //fprintf(stderr, "%d - %d\n", (tmp->TID), (uthread_t)(intptr_t)arg);
                return 1;

        }

//        fprintf(stderr, "%d - %d\n", (tmp->TID), (uthread_t)(intptr_t)arg);

        return 0;

}

int find_Prnt_T(void *data, void *arg){

        TCB *tmp = data;

        //Child ID of Parent in Queue matching
        if(tmp->child_tid == (uthread_t)(intptr_t)arg) {
                //fprintf(stderr, "%d - %d\n", (tmp->TID), (uthread_t)(intptr_t)arg);
                return 1;

        }

        return 0;

}
/*
   uthread_exit is called when the current active is finished execution
   We need to collect the value, set the status and put it in Zombie Queue
   We do not Delete the data

 */

void uthread_exit(int retval)
{

        //The return value @retval is to be collected
        activeT->retval = retval;
        activeT->status = ZOMBIE;


        //unblock parent once child is dead
        TCB *Prnt = NULL;

        preempt_disable();

        //The child that just died. We look for any parent that owns this child
        queue_iterate(blocked_queue, find_Prnt_T, (void *)(intptr_t)activeT->TID, (void **)&Prnt); //PASS IN ID OF CHILD

        //if parent in blocked_queue has child that died
        if(Prnt) {

                //if(Prnt->status != -1) {

                //printf("unblocked %d\n", Prnt->TID);
                Prnt->status = READY;         //unblock the parent
                queue_enqueue(ready_queue, Prnt);         //put it back in ready queue because it used to be an active process

                queue_delete(blocked_queue, Prnt);         //delete from zombie queue

//                preempt_enable();

                //uthread_yield();
                //}

        }
        //else{


        //Save the pointer to the currently active thread
        TCB *tmp = activeT;
        //put in ZOMBIE queue
        queue_enqueue(zombie_queue, tmp);

        //Last thread in Queue
        if(queue_dequeue(ready_queue, (void**)&activeT)<0) {
                preempt_enable();
                return;
        }


        //switch context with next element in ready queue
        uthread_ctx_switch(tmp->cntxt, activeT->cntxt);
        //}
        preempt_enable();

}

/*

   Initally Main is the current active thread
   Parent Join Child- Waitpid implementation
   We block the parent (Calling thread) and wait for child to exit
   Return the return value of child

 */

int uthread_join(uthread_t tid, int *retval)
{
        /* TODO Phase 2 */
        /* TODO Phase 3 */

        preempt_disable();


        TCB *tmp = NULL;
        //ACCESSING DATA

        //If it is the main Thread or thread is trying to join itself return -1
        if(tid == 0 || activeT->TID == tid) {
                return -1;
        }
        //uthread_ctx_destroy_stack((void *)tmp->stack_ptr);
        //free(tmp);

        //Find Thread in Ready Queue
        queue_iterate(ready_queue, find_thread, (void *)(intptr_t)tid, (void **)&tmp);

        //Child has not run yet
        if(tmp) {

                // restriction: a thread already joined, cannot be joined twice
                if(activeT->child_tid != -1) {
                        preempt_enable(); //DONE ACCESS
                        return -1;
                }

                //Block the parent because child has not returned
                activeT->status = BLOCKED;

                //setting parent's child_tid
                activeT->child_tid = tid;

                //The parent is blocked
                queue_enqueue(blocked_queue, activeT);

                //Remove Active Thread from Ready Queue
                //queue_delete(ready_queue, activeT);

                //wait for Child to finish. To switch context
                //preempt_enable();
                uthread_yield();


                if (retval) {
                        //Collect return value
                        *retval = tmp->retval;
                }



        }
        //Re-enable it
        preempt_enable();


/*
        while(1) {
        Phase 2

   //        if there are no more threads which are ready to run in system
                if(queue_length(ready_queue)==0) {
                        break;


                }
                else{ //Otherwise simply yield to next available thread

                        uthread_yield();

                }
        }


 */

        return 0;

}
