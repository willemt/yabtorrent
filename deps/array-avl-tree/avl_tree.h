#ifndef AVL_TREE_H
#define AVL_TREE_H

typedef struct {
    void* key;
    void* val;
} node_t;

typedef struct {
    /* size of array */
    int size;
    int count;
    long (*cmp)(
        const void *e1,
        const void *e2);
    node_t *nodes;
} avltree_t;

typedef struct {
    int current_node;
} avltree_iterator_t;

avltree_t* avltree_new(long (*cmp)(const void *e1, const void *e2));

void* avltree_remove(avltree_t* me, void* k);

int avltree_count(avltree_t* me);

int avltree_size(avltree_t* me);

int avltree_height(avltree_t* me);

void avltree_empty(avltree_t* me);

void avltree_insert(avltree_t* me, void* k, void* v);

void* avltree_get(avltree_t* me, const void* k);

void* avltree_get_from_idx(avltree_t* me, int idx);

/**
 * Rotate on X:
 * Y = X's parent
 * Step A: Y becomes left child of X
 * Step B: X's left child's becomes Y's right child */
void avltree_rotate_left(avltree_t* me, int idx);

/**
 * Rotate on X:
 * Y = X's left child
 * Step A: X becomes right child of X's left child
 * Step B: X's left child's right child becomes X's left child */
void avltree_rotate_right(avltree_t* me, int idx);


/**
 * Initialise a new hash iterator over this hash
 * It is NOT safe to remove items while iterating.  */
void avltree_iterator(avltree_t * h, avltree_iterator_t * iter);

/**
 * Iterate to the next item on an iterator
 * @return next item key from iterator */
void *avltree_iterator_next(avltree_t * h, avltree_iterator_t * iter);

/**
 * Iterate to the next item on an iterator
 * @return next item value from iterator */
void *avltree_iterator_next_value(avltree_t * h, avltree_iterator_t * iter);

int avltree_iterator_has_next(avltree_t * h, avltree_iterator_t * iter);

void* avltree_iterator_peek_value(avltree_t * h, avltree_iterator_t * iter);

void* avltree_iterator_peek(avltree_t * h, avltree_iterator_t * iter);

#endif /* AVL_TREE_H */
