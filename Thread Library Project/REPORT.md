# Phase 1: queue API

Creating a Linked List
We first started by creating a linked list to create the queue. This linked
list consisted of a struct Node that took care of each nodes in the queue.
Another struct called queue was also created to point to the head and tail of
the queue and it also stored the data of the node.

# Functions

queue_create(), queue_destroy(), and queue_length() were straight forward
functions and we simply followed the directions given to us in the instructions.
queue_enqueue(), queue_dequeue(), and queue_iterate() were more tricky because
in enqueue and dequeue we had to make sure that we accounted for every cases
such as dequeuing an empty or NULL queue and enqueuing when the queue is not
empty or empty and also if the queue data, newNode, or the queue itself are
NULL. The iterate function was the most difficult for us because we have never
used callback functions. The queue_delete() function was also difficult for
us to implement. We basically ended up iterating through the queue and
checking if the data we were looking to delete matched the data to any
node in the queue, assigned the pointer to the data then we deleted that node
and moved on to the next node.


# Phases 2 and 3: uthread API

We started by creating a TCB struct which includes:
TID: stores the thread ID of the thread
state: holds the current state of the thread
stack_ptr: pointer to the top of the stack
context: this is an object which executes the threads
retval: stores the return value of the threads

We created a function called initmainthread() where we initialized the main
 thread. In this function we set the main thread to READY, set the TID to 0,
 allocated space for the stack and the context. We also initialize the queues.

uthread_create(): when its called for the first time, it goes in to the
initmainthread() function and initializes the main thread. When its called
after the first time, it creates a new thread and enqueues it to the ready
thread.

uthread_self(): Returns the TID of the current active thread

uthread_yield(): Switches the context of the next thread in the ready queue
with the current thread. Pushes the current thread into the ready queue.

uthread_exit(): phase 2 -> once a thread has exited, its status becomes a
ZOMBIE and we get its return value. In phase 3, we iterated through the
blocked queue to find the parent that owns the child that died. If we
find the parent, we enqueue it to the ready queue and delete it from
the blocked queue. At the end, we switch context of current thread with
the next thread in the ready queue.

uthread_join(): In phase 2, it was just a while loop that kept calling the
yield function as long as the length of the queue wasn't 0. If the length of
the queue is 0, it broke. In Phase 3, we blocked the parent thread and wait
for the child to finish. Once the child finishes, we capture its return value.
 We also have cases so that if the main thread or if a thread tries to join
 itself, it returns 1. We also make sure that once a thread has joined, it
 cannot join again.

# Phase 4: preemption

preempt_enable(): we create a sigset_t object to store blocked and unblocked
signals. We also added a SIGVTALRM and called sigprocmask() with SIG_BLOCK to
unblock the alarm signal.

preempt_disable(): we create a sigset_t object again. We also added a SIGVTALRM
and called sigprocmask() with SIG_BLOCK to block the alarm signal

preempt_start():
A sigaction object is created to define what happens when it receives a signal.
An itimerval object was created to store the timer values. The interval between
the alarms was set as 10,000 microseconds which equals 0.01 seconds. This allows
the timer to go off 100 times per second.  


#Testing

We were given a basic tester for the queue and we modified it to make sure that
we could enqueue, dequeue, destroy and check the length of the queue. We test
the enqueue and dequeue by passing in NULL as both arguments and also by
checking whether or not the size of the queue increased/decreased after we ran
the functions. We also passed empty and non-empty queues to test the destroy
function. Given the uthread_hello and the uthread_yield tester files, we were
able to test the ability of our uthread program to create threads and yield
threads.


Sources:
https://www.gnu.org/software/libc/manual/html_mono/libc.html#System-V-contexts
https://www.gnu.org/software/libc/manual/html_mono/libc.html#Blocking-Signals
https://www.gnu.org/software/libc/manual/html_mono/libc.html#Signal-Actions
https://www.gnu.org/software/libc/manual/html_mono/libc.html#Setting-an-Alarm
http://tldp.org/HOWTO/Program-Library-HOWTO/static-libraries.html
