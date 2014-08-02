#ifndef MEANQUEUE_H
#define MEANQUEUE_H

typedef struct
{
    int *vals;
    int size;
    int idxCur;
    int valSum;
    float mean;
} meanqueue_t;

/**
 * Create of queue of size 0 */
void *meanqueue_new(
    const int size
);

void meanqueue_free(
    meanqueue_t * qu
);

/**
 * Add an int to the queue.
 * If the queue can't hold all the ints, the oldest int is removed */
void meanqueue_offer(
    meanqueue_t * qu,
    const int val
);

/**
 * @return mean of ints within queue */
float meanqueue_get_value(
    meanqueue_t * qu
);

#endif /* MEANQUEUE_H */
