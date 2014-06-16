#ifndef BAG_H
#define BAG_H

typedef struct {
    void **array;
    int size;
    int count;
} bag_t;

/**
 * Init a bag and return it. Malloc space for it.
 * @return initialised bag */
bag_t *bag_new();

/**
 * Free memory held by bag */
void bag_free(bag_t * b);

void bag_put(bag_t * b, void* i);

/**
 * Remove one random item.
 * @return one random item */
void *bag_take(bag_t * b);

int bag_count(bag_t * b);

#endif /* BAG_H */
