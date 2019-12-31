#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "queue.h"

typedef struct dummy {
        char data;
        int z;
        int a;
} dummy;

static int print_queue(void *data, void *arg)
{
        printf("%d \n", ((dummy *)data)->z);
        printf("%c \n", ((dummy *)data)->data);
        printf("%d \n", ((dummy *)data)->a);

        return 0;
}

void test_enqueue(void)
{
        queue_t q;

        q = queue_create();

        int x = queue_enqueue(NULL, NULL);
        assert(x == -1);
        printf("Assert NULL for queue and data success: %d \n", x);


        int y = queue_enqueue(q, NULL);
        assert(y == -1);
        printf("Assert NULL for data success: %d \n", y);


        dummy *test;
        test = malloc(sizeof(dummy));

        test->z = 4;
        test->data = 'h';
        queue_enqueue(q, test);

        queue_iterate(q, print_queue, NULL, NULL);
        printf("Number of Elements in the queue: %d\n", queue_length(q));

}


void test_dequeue(void)
{
        queue_t q;
        q = queue_create();
        int data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        dummy *temp;
        temp = malloc(sizeof(dummy));

        int x = queue_dequeue(NULL, NULL);
        assert(x == -1);
        printf("Dequeue: Assert NULL for queue and data success: %d \n", x);

        int y = queue_dequeue(q, NULL);
        assert(y == -1);
        printf("Dequeue: Assert NULL for data success: %d \n", y);

        for (int i = 0; i < 10; i++) {
                queue_enqueue(q, &data[i]);
        }

        temp->z = 4;
        temp->a = 9;

        printf("B Number of Elements in the queue: %d\n", queue_length(q));

        queue_dequeue(q, (void **)&temp->z);
        queue_dequeue(q, (void **)&temp->a);

        printf("A Number of Elements in the queue: %d\n", queue_length(q));

}

void test_destroy()
{
        queue_t q;
        dummy *temp;
        temp = malloc(sizeof(dummy));
        q = queue_create();

        temp->z = 4;
        temp->a = 9;

        queue_enqueue(q, &temp->z);
        queue_enqueue(q, &temp->a);

        //non-empty queue
        int z = queue_destroy(q);
        assert(z == -1);
        printf("Successful in destroying a non-empty queue %d\n", z);

        //empty queue
        queue_delete(q, &temp->z);
        queue_delete(q, &temp->a);
        int u = queue_destroy(q);
        assert(u == 0);
        printf("Successful in destroying an empty queue %d\n", u);


}

/* Callback function that increments items by a certain value */
static int inc_item(void *data, void *arg)
{
        int *a = (int*)data;
        int inc = (intptr_t)arg;
        *a += inc;

        return 0;
}

/* Callback function that finds a certain item according to its value */
static int find_item(void *data, void *arg)
{
        int *a = (int*)data;
        int match = (intptr_t)arg;

        if (*a == match)
                return 1;

        return 0;
}

void test_iterator(void)
{
        queue_t q;
        int data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        int i;
        int *ptr;

        /* Initialize the queue and enqueue items */
        q = queue_create();
        for (i = 0; i < 10; i++)
                queue_enqueue(q, &data[i]);

        /* Add value '1' to every item of the queue */
        queue_iterate(q, inc_item, (void*)1, NULL);
        assert(data[0] == 2);

        /* Find and get the item which is equal to value '5' */
        queue_iterate(q, find_item, (void*)5, (void*)(intptr_t)&ptr);
        assert(*ptr == 5);

}

int main()
{

        test_enqueue();

        test_dequeue();

        test_iterator();

        //test delete and destroy
        test_destroy();



}
