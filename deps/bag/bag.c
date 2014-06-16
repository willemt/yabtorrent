#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bag.h"

bag_t *bag_new()
{
    bag_t *b;

    b = malloc(sizeof(bag_t));
    b->size = 10;
    b->array = malloc(sizeof(void *) * b->size);
    b->count = 0;
    return b;
}

static void __ensurecapacity(bag_t * b)
{
    int ii;
    void **array_n;

    if (b->count < b->size)
	return;

    /* double capacity */
    b->size *= 2;
    array_n = malloc(sizeof(void *) * b->size);

    /* copy old data across to new array */
    for (ii = 0; ii < b->count; ii++)
    {
	array_n[ii] = b->array[ii];
        assert(b->array[ii]);
    }

    /* swap arrays */
    free(b->array);
    b->array = array_n;
}

void bag_put(bag_t * b, void* item)
{
    __ensurecapacity(b);
    b->array[b->count++] = item;
}

void* bag_take(bag_t * b)
{
    int idx;
    void* i;

    if (0 == b->count) return NULL;

    idx = rand() % b->count;
    i = b->array[idx];
    b->array[idx] = b->array[b->count-1];
    b->count -= 1;
    return i;
}

int bag_count(bag_t * b)
{
    return b->count;
}

void bag_free(bag_t * b)
{
    free(b->array);
    free(b);
}

