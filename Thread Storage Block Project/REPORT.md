Overview:

We created two features to extend the functionality of the POSIX pthreads
library: a semaphore struct that is used for thread synchronization, and a
dedicated private memory page for each thread. The Semaphore works by keeping
a count of available resources and a queue of threads that would like to
claim a resource. The queue was provided for us in queue.o. A thread will
try to claim a resource, and if the count is zero then it will be enqueued
and then blocked until the count is not zero anymore. When the thread is
unblocked it will decrement the count of resources. When gives up control,
it increments the count, and then dequeues and unblocks the first thing in
the queue.

The memory page associated with each thread is stored in a storage struct.
It contains a pointer that has the address of the page in memory it was
assigned, using mmap. Mmap assigns this page as protected, so the pages can
only be read or written to with our tps_read and tps_write functions, which
temporarily disable the protections. The storage pages for each thread are
stored in a queue.

SEMAPHORE:

Sem_down():

A thread calls sem_down() when it wants to grab a shared resource. It first
calls enter_critical_section(), provided in thread.o, which ensures mutual
exclusion from other threads. It then checks the count of the semaphore that
was passed in, and if there is no resources then it enqueues itself in the
semaphore’s queue, and then blocks itself. This ensures that the thread can’t
be run again until it is unblocked by another thread. Calling block removes
the thread from the critical section. After the count is checked, the count
is decremented and exit_critical_section() is called. This is because the
semaphore’s count will either have been greater than zero, or the thread
calling sem_down() will have just been unblocked, which can only happen
after the semaphore’s count was incremented. Decrementing the count
indicates that the thread has taken up a resource, so if there is none left
and another thread calls sem_down(), it will be blocked until there is a
resource available.

Sem_up():

Sem_up does the opposite of sem_down(). It is called when a thread is done
using a resource and wants to free the resource up. It enters a critical
section, increments the semephore’s count, and then dequeues the first thing in
the semephore’s queue and unblocks it, if there was anything in the queue. It
then exits the critical section.

# TPS:


## Struct storage:

The storage struct is what gets created for a threads protected memory page.
It contains the owner’s thread ID, and a page struct. The page struct contains
a pointer to the start address of the page, and a count of how many threads
are pointing to this address. This is used for a copy-on-write feature
explained later in the report.


## tps_create()/tps_destroy():

tps_create() mallocs space for a storage and page struct, and then associates
memory for the page to the address field in the page struct using mmap(). We
call mmap() with 4096 bytes for the size of the memory mapping, and with the
PROT_NONE argument, which makes the page inaccessible. The count field of the
page struct contained in the storage struct is set to 1, and the ID field is
set to the calling threads ID, before it is stored in the queue. This is all
done within a critical section. Tps-destroy() iterates through the queue,
looking for the thread ID of the calling thread, and disassociates the memory
space with the page’s address pointer with munmap(), before freeing the page
pointer. It only does this when there is only one thing pointing to that page.
It then decrements the count field of the page and removes the thread from the
queue.

## tps_read()/tps_write:

### Tps_read()
finds the calling thread’s storage struct using queue_iterate() and
a custom find thread ID function, then changes the permissions to allow a read
on the address field with mprotect(). It then reads from the address page to a
buffer using memcpy, and then changes back the permissions.    	

### Tps_write()
functions the same as tps_read() except it writes from a buffer to the address
page, and before writing it checks to see if there are more than
one thread pointing to the page that will be written to. This is because of
the copy-on-write feature that we implemented. If the count is more than 1,
it is because a thread storage area was created by cloning an already created
storage, but since they would be contain the same thing before writing to
either one, every cloned thread just points to the original thread’s page
before writing.  When a write does happen was the tricky part. We first
decided to iterate through the queue and give every single page that shared
the address it’s new page. This could be really wasteful so instead we decided
to create a page for the calling thread update the count for all those that
were sharing the page (which is no longer shared with the current thread) and
continue the write operation on the thread’s tps.

## Tps_clone():

Tps_clone() creates a new TPS for the calling thread if it does not already
have one, and then points the cloning thread’s page pointer to the cloned
thread’s page.  It then increments both threads count, to indicate that one
more thing is pointing to the original page. This is done instead of just
copying over the contents of the original thread’s page to the cloned thread’s
page because of the copy-on-write feature. Before either the cloning or cloned
page is written to, they contain the same contents, so just point to the same
page. Our write function checks that a page has clones and makes actual copies
before the writing is done. Our destroy function only destroys the actual
memory page if there are no other threads pointing to it but the calling
thread. This is to avoid a situation where a thread is clone but then is
deleted before the cloning thread has a write.

# Segmentation fault handler:

Since our TPS pages cannot be accessed outside of our own read and write
functions, when this does happen a segmentation fault occurs. We set up a
custom handler for this called segv_handler(). In tps_init(), if the segv
argument is given then sigaction() is called to change the response to the
SIGBUS and SIGSEV signals to our custom handler. Our handler gets the address
of the beginning of the page that caused the signal to be sent, and iterates
through the queue to see if the address matches any of the threads’
page address. If it does, then a custom print statement is printed. The
default signal handlers are then restored and the program crashes normally.

# Testing:

Testing for the semaphore phase of this project was done with the given tester
files  sem_count, sem_buffer, and sem_prime. Sem_count had two threads go back
and forth, incrementing and printing a shared int variable until 19. Sem_buffer
tested the producer consumer problem, where one thread is writing to a circular
buffer and another thread is reading from the same circular buffer. Sem_prime
has a similar producer and consumer thread, but in addition a filter thread,
that filters the numbers produced by the producer thread and only keeps prime
numbers. Our program passed all given test files.

Testing for the TPS phase was done with the given tps.c, as well as tpsTest.c,
and tpsTest2.c, which are modified versions of the given test file, used to
test more intricate cases. tps.c creates two threads, and makes a thread
storage area for thread 2, and clones thread 2’s storage area for thread 1.
A message is written to both thread pages and then read, and compared to the
correct message. Passing this tester indicated that the threads successfully
created a TPS area, cloned a TPS area, wrote to, and read from a TPS page.

TpsTest.c is modified to test bad reads, writes, and destroys, reading and
writing from offsets, and also whether the copy-on-write feature worked as
intended. In thread 1, before a TPS area is created, we try to read, write,
and destroy thread 1’s tps, to demonstrate that it would return -1. We also
print the address of of thread 2’s page, and the address of thread 1’s page
after a clone and before a write, and then the address of thread 1’s page after
a write. This is to show that an actual copy is made for thread 1 only after a
write is done. We didn’t know how to print the address of a thread’s tps page
outside of a function defined in tps.h, and we also didn’t know how else to
demonstrate that the threads only make actual copies after a write besides
comparing the address pointers. In order to print the addresses, we made a
special print function that gets called if tps_init() is called with an
argument of -1. Our reasoning was that tps_init() would never get called
with an argument of   -1 normally, so we could use it for testing purposes.
This was a workaround to a tricky problem for us, and we recognize that it is
probably not the best practice. In tpsTest.c, we also write to and read from
the tps page of thread 1 at an offset, as well as the entire page, to ensure
that our read and write functions were working properly.

tpsTest2.c tested for two specific cases. It first created scenario where
thread 1 cloned thread 2, and then before the copy-on-write thread 2 was
destroyed. After passing this case we forced a SIGSEV signal to be sent by
writing to the last TPS page mmap() allocated outside of our tps_write()
function. This caused our custom handler to print its message before regularly
seg faulting.  To get the latest TPS page that mmap allocated, we used code
provided to us. This code overloaded the mmap function to keep track of the
address that mmap returned. We also had to create a custom rule for our
makefile in order to have GCC use the custom mmap wrapper instead of the
original mmap().

tpsClonetest: Used to test the clone and destroy feature. First we create T1,
which creates T2. T2 creates a tps then creates T3 which clones this TPS then
destroys it. We go back to T2 which is unaffected and just passes it on to T1
which clones T2 and makes sure the data is still there. Then we return to main
where it is successful if it went past the assert.
