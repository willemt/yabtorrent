#ifndef LINKED_LIST_HASHMAP_H
#define LINKED_LIST_HASHMAP_H

typedef unsigned long (*func_longhash_f) (const void *);

typedef long (*func_longcmp_f) (const void *, const void *);

typedef struct
{
    void *key;
    void *val;
} hashmap_entry_t;

typedef struct
{
    int count;
    int arraySize;
    void *array;
    func_longhash_f hash;
    func_longcmp_f compare;
} hashmap_t;

typedef struct
{
    int cur;
    void *cur_linked;
} hashmap_iterator_t;

hashmap_t *hashmap_new(
    func_longhash_f hash,
    func_longcmp_f cmp,
    unsigned int initial_capacity
);

/**
 * @return number of items within hash */
int hashmap_count(const hashmap_t * hmap);

/**
 * @return size of the array used within hash */
int hashmap_size(
    hashmap_t * hmap
);

/**
 * Empty this hash. */
void hashmap_clear(
    hashmap_t * hmap
);

/**
 * Free all the memory related to this hash. */
void hashmap_free(
    hashmap_t * hmap
);

/**
 * Free all the memory related to this hash.
 * This includes the actual h itself. */
void hashmap_freeall(
    hashmap_t * hmap
);

/**
 * Get this key's value.
 * @return key's item, otherwise NULL */
void *hashmap_get(
    hashmap_t * hmap,
    const void *key
);

/**
 * Is this key inside this map?
 * @return 1 if key is in hash, otherwise 0 */
int hashmap_contains_key(
    hashmap_t * hmap,
    const void *key
);

/**
 * Remove the value refrenced by this key from the hash. */
void hashmap_remove_entry(
    hashmap_t * hmap,
    hashmap_entry_t * entry,
    const void *key
);

/**
 * Remove this key and value from the map.
 * @return value of key, or NULL on failure */
void *hashmap_remove(
    hashmap_t * hmap,
    const void *key
);

/**
 * Associate key with val.
 * Does not insert key if an equal key exists.
 * @return previous associated val; otherwise NULL */
void *hashmap_put(
    hashmap_t * hmap,
    void *key,
    void *val
);

/**
 * Put this key/value entry into the hash */
void hashmap_put_entry(
    hashmap_t * hmap,
    hashmap_entry_t * entry
);

void* hashmap_iterator_peek(
    hashmap_t * hmap,
    hashmap_iterator_t * iter);

void* hashmap_iterator_peek_value(
    hashmap_t * hmap,
    hashmap_iterator_t * iter);

int hashmap_iterator_has_next(
    hashmap_t * hmap,
    hashmap_iterator_t * iter
);

/**
 * Iterate to the next item on a hash iterator
 * @return next item key from iterator */
void *hashmap_iterator_next(
    hashmap_t * hmap,
    hashmap_iterator_t * iter
);

/**
 * Iterate to the next item on a hash iterator
 * @return next item value from iterator */
void *hashmap_iterator_next_value(
    hashmap_t * hmap,
    hashmap_iterator_t * iter);

/**
 * Initialise a new hash iterator over this hash
 * It is safe to remove items while iterating.  */
void hashmap_iterator(
    hashmap_t * hmap,
    hashmap_iterator_t * iter
);

/**
 * Increase hash capacity.
 * @param factor : increase by this factor */
void hashmap_increase_capacity(
    hashmap_t * hmap,
    unsigned int factor);

#endif /* LINKED_LIST_HASHMAP_H */
