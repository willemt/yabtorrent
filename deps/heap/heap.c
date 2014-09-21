
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "heap.h"

#define DEFAULT_CAPACITY 13

static int __child_left(const int idx)
{
    return idx * 2 + 1;
}

static int __child_right(const int idx)
{
    return idx * 2 + 2;
}

static int __parent(const int idx)
{
    return (idx - 1) / 2;
}

void heap_init(heap_t* h,
               int (*cmp) (const void *,
                           const void *,
                           const void *udata),
               const void *udata,
               void **array,
               unsigned int size
               )
{
    h->cmp = cmp;
    h->udata = udata;
    h->size = size;
    h->count = 0;
    h->array = array;
}

heap_t *heap_new(int (*cmp) (const void *,
                             const void *,
                             const void *udata),
                 const void *udata)
{
    heap_t *h = malloc(sizeof(heap_t));

    if (!h)
        return NULL;

    void** array = malloc(sizeof(void *) * DEFAULT_CAPACITY);

    if (!array)
    {
        free(h);
        return NULL;
    }

    heap_init(h, cmp, udata, array, DEFAULT_CAPACITY);

    return h;
}

void heap_free(heap_t * h)
{
    free(h->array);
    free(h);
}

/**
 * @return 0 on success; -1 otherwise */
static int __ensurecapacity(heap_t * h)
{
    if (h->count < h->size)
        return 0;

    h->size *= 2;

    void **new_array = malloc(sizeof(void *) * h->size);

    if (!new_array)
        return -1;

    /* copy old data across to new array */
    unsigned int i;
    for (i = 0; i < h->count; i++)
        new_array[i] = h->array[i];

    /* swap arrays */
    free(h->array);
    h->array = new_array;
    return 0;
}

static void __swap(heap_t * h, const int i1, const int i2)
{
    void *tmp = h->array[i1];

    h->array[i1] = h->array[i2];
    h->array[i2] = tmp;
}

static int __pushup(heap_t * h, unsigned int idx)
{
    /* 0 is the root node */
    while (0 != idx)
    {
        int parent = __parent(idx);

        /* we are smaller than the parent */
        if (h->cmp(h->array[idx], h->array[parent], h->udata) < 0)
            return -1;
        else
            __swap(h, idx, parent);

        idx = parent;
    }

    return idx;
}

static void __pushdown(heap_t * h, unsigned int idx)
{
    while (1)
    {
        unsigned int childl, childr, child;

        childl = __child_left(idx);
        childr = __child_right(idx);

        if (childr >= h->count)
        {
            /* can't pushdown any further */
            if (childl >= h->count)
                return;

            child = childl;
        }
        /* find biggest child */
        else if (h->cmp(h->array[childl], h->array[childr], h->udata) < 0)
            child = childr;
        else
            child = childl;

        /* idx is smaller than child */
        if (h->cmp(h->array[idx], h->array[child], h->udata) < 0)
        {
            __swap(h, idx, child);
            idx = child;
            /* bigger than the biggest child, we stop, we win */
        }
        else
            return;
    }
}

static int __heap_offerx(heap_t * h, void *item)
{
    h->array[h->count] = item;

    /* ensure heap properties */
    __pushup(h, h->count++);
    return 0;
}

int heap_offerx(heap_t * h, void *item)
{
    if (!item)
        return -1;
    if (h->count == h->size)
        return -1;
    return __heap_offerx(h, item);
}

int heap_offer(heap_t * h, void *item)
{
    if (!item)
        return -1;
    if (-1 == __ensurecapacity(h))
        return -1;
    return __heap_offerx(h, item);
}

void *heap_poll(heap_t * h)
{
    if (0 == heap_count(h))
        return NULL;

    void *item = h->array[0];

    h->array[0] = NULL;
    __swap(h, 0, h->count - 1);
    h->count--;

    if (h->count > 0)
        __pushdown(h, 0);

    return item;
}

void *heap_peek(heap_t * h)
{
    if (0 == heap_count(h))
        return NULL;

    return h->array[0];
}

void heap_clear(heap_t * h)
{
    h->count = 0;
}

/**
 * @return item's index on the heap's array; otherwise -1 */
static int __item_get_idx(heap_t * h, const void *item)
{
    unsigned int idx;

    for (idx = 0; idx < h->count; idx++)
        if (0 == h->cmp(h->array[idx], item, h->udata))
            return idx;

    return -1;
}

void *heap_remove_item(heap_t * h, const void *item)
{
    int idx = __item_get_idx(h, item);

    if (idx == -1)
        return NULL;

    /* swap the item we found with the last item on the heap */
    void *ret_item = h->array[idx];
    h->array[idx] = h->array[h->count - 1];
    h->array[h->count - 1] = NULL;

    h->count -= 1;

    /* ensure heap property */
    __pushup(h, idx);

    return ret_item;
}

int heap_contains_item(heap_t * h, const void *item)
{
    return __item_get_idx(h, item) != -1;
}

int heap_count(heap_t * h)
{
    return h->count;
}

int heap_size(heap_t * h)
{
    return h->size;
}

/*--------------------------------------------------------------79-characters-*/
