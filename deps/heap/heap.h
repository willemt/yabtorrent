#ifndef HEAP_H
#define HEAP_H

typedef struct {
    void **array;
    /* size of array */
    unsigned int size;
    /* items within heap */
    unsigned int count;
    /**  user data */
    const void *udata;
    int (*cmp) (const void *, const void *, const void *);
} heap_t;

/**
 * Init a heap and return it. Malloc space for it.
 *
 * @param cmp Callback used to determine the priority of the item
 * @param udata Udata passed through to compare callback
 * @return initialised heap */
heap_t *heap_new(int (*cmp) (const void *,
			     const void *,
			     const void *udata), const void *udata);

/**
 * Free memory held by heap */
void heap_free(heap_t * hp);

/**
 * Add this value to the heap.
 * @param item Item to be added to the heap */
void heap_offer(heap_t * hp, void *item);

/**
 * Remove the top value from this heap.
 * @return top item of the heap */
void *heap_poll(heap_t * hp);

/**
 * @return item on the top of the heap */
void *heap_peek(heap_t * hp);

/**
 * Clear all items from the heap.
 * Only use if item memory is managed outside of the heap. */
void heap_clear(heap_t * hp);

/**
 * How many items are there in this heap?
 * @return number of items in heap */
int heap_count(heap_t * hp);

/**
 * The heap will remove this item
 * @param item Item that is to be removed
 * @return item to be removed */
void *heap_remove_item(heap_t * hp, const void *item);

/**
 * The heap will remove this item
 * @param item Item to be removed
 * @return 1 if the heap contains this item, 0 otherwise */
int heap_contains_item(heap_t * hp, const void *item);

#endif /* HEAP_H */
