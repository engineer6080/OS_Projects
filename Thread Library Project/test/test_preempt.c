#include <stdio.h>
#include <unistd.h>
#include <uthread.h>
#include <stdlib.h>

/*
   Print "Hello world" even though it is an infinite loop,
   but the preempt should disable it
 */


//Runs in infinte loop
int infinite_func (void * arg)
{
        while (1)
                continue;
        return 0;

}

//Thread prints Hello World and exits
int single (void * arg)
{
        printf("Hello World");
        exit(0);
        return 0;
}

int main (void)
{
        uthread_t tid1; //Create the first thread.
        tid1 = uthread_create(infinite_func, NULL);

        //Create single thread
        uthread_create(single, NULL);

        //Join in the first thread.
        uthread_join(tid1, NULL);

        return 0;


}
