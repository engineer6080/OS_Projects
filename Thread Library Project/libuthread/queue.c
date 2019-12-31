#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "queue.h"

//Linked list node
typedef struct Node {
        void *data;
        struct Node *next;
} Node;

//node pointing to the first and the last element of the queue
typedef struct queue { //it's called queue in the .h file
        Node *head; //node has data and next
        Node *tail; //node has data and next also
        int size; //size
} queue;


//Create a new queue and return the queue
queue_t queue_create(void)
{
        queue_t q;
        q = malloc(sizeof(struct queue));

        //Return NULL if malloc fails
        if (q == NULL) {
                return NULL;
        }

        //intialize values to NULL
        q->head = NULL;
        q->tail = NULL;

        //init size to 0
        q->size = 0;

        return q;

}

//Destroy the queue
int queue_destroy(queue_t queue)
{
        // Deallocate the memory associated to the queue object pointed by @queue.
        //If it's not empty or it's NULL
        if (queue->size != 0 || !queue) {
                return -1;
        }
        //if it's empty then free it
        free(queue);

        //successfully destroyed
        return 0;

}

/*
        Create and add new node
        We are adding to the BACK
        enqueue->[tail][][][head] -> dequeue
        head
        going down
        tail
 */
int queue_enqueue(queue_t queue, void *data)
{

        Node* newNode;
        newNode = malloc(sizeof(struct Node));

        //if malloc failed or the data or the queue itself is NULL, return -1
        if (newNode == NULL || (data == NULL) || (queue == NULL)) {
                return -1;
        }

        //set the data
        newNode->data = data;
        //end of queue so there is no NEXT
        newNode->next = NULL;

        //EMPTY QUEUE
        if((queue->head == NULL)) {

                //beginning and end of queue because only element
                queue->head = newNode;
                queue->tail = newNode;

        }
        else{ //NOT EMPTY
              //previous tail's next
                queue->tail->next = newNode;
                //New tail
                queue->tail = newNode;
        }

        //increment size of queue
        queue->size++;

        //if succesful
        return 0;

}

int queue_dequeue(queue_t queue, void **data)
{

        //enqueue->[tail][][][head] -> dequeue

        //Return: -1 if @queue or @data are NULL, or if the queue is empty
        if(queue == NULL  || data == NULL || queue->head == NULL) { //can check head or tail for NULL (initial values)
                return -1;
        }

        //* Remove the oldest item of queue @queue and assign this item (the value of a
        //* pointer) to @data.

        //WE ARE SENDING BACK THE Dequeued ITEM BACK TO THE USER

        //Data that is kept for now
        *data = queue->head->data;

        //save pointer to node after head
        Node* temp = queue->head->next;

        //Free Head
        free(queue->head);

        //NEW HEAD
        queue->head = temp;

        queue->size--; //decrement size

        return 0;

}

int queue_delete(queue_t queue, void *data)
{

        //if queue or data is NULL then return -1
        if(queue == NULL || data == NULL) {
                return -1;
        }

        //Node iterator
        Node* itr = queue->head;

        //DON'T FREE THE DATA

        //if the head itself has the data
        if(itr->data == data) {

                void* sav_next = queue->head->next; //save pointer to element after head

                free(queue->head); //free head

                queue->head = sav_next; //new head

                queue->size--;

                return 0;

        }

        // iterate through queue until we reach the end
        for(itr = queue->head->next; itr != NULL; itr = itr->next) {

                //check the next one so we can have access to the current one
                //if it exists
                if(itr->next) {
                        //found data
                        if(itr->next->data == data) {

                                //pointer to node we are about to delete
                                void* toDel = itr;

                                //[itr][itr->next][itr->next->next]
                                if((itr->next)->next) { //check if it exists
                                        itr->next = (itr->next)->next; //skip the one that's going to be deleted
                                }
                                else{ //then we know itr->next is the tail
                                        queue->tail = itr; //assign new tail
                                }

                                free(toDel); //free

                                queue->size--; //decrement size

                                return 0;


                        }
                }

        }

        //went through queue- not found
        return -1;


}

/*
   Cool function to iterate and perform custom operation on each element in queue
   return 1 in end of custom function to stop iterating and return data if the data is not NULL. (ex FIND)
   else just perform operation on all and return 0

 */

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{

        //Iterator
        Node* itr;

        for(itr = queue->head; itr != NULL; itr = itr->next) {

                //get return value of func. Accepts data and arg
                int retV = func(itr->data, arg);
                //function should return 1 when we want to stop iterating
                if(retV == 1) { //1 to stop iterating at this partricular item
                        if(itr->data != NULL) {

                                *data = itr->data;

                        }
                        else{
                                return -1;
                        }

                }
        }

        return 0;
}

//Return queue length
int queue_length(queue_t queue)
{
        if(queue) {
                return queue->size;
        }
        //queue is NULL
        else{
                return -1;
        }


}
