
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <stdio.h>

#include "pseudolru.h"

#define LEFT 0
#define RIGHT 1

typedef struct tree_node_s tree_node_t;

struct tree_node_s
{
    tree_node_t *left, *right;
    void *key, *value;
    int bit;
};

pseudolru_t *pseudolru_new(
    int (*cmp) (const void *,
                const void *)
)
{
    pseudolru_t *me = calloc(1,sizeof(pseudolru_t));
    memset(me, 0, sizeof(pseudolru_t));
    me->cmp = cmp;
    return me;
}

static void __free_node(
    tree_node_t * node
)
{
    if (node)
    {
        __free_node(node->left);
        __free_node(node->right);
        free(node);
    }
}

void pseudolru_free(
    pseudolru_t * me
)
{
    __free_node(me->root);
    free(me);
}

static tree_node_t *__init_node(
    void *key,
    void *value
)
{
    tree_node_t *new = malloc(sizeof(tree_node_t));
    new->left = new->right = NULL;
    new->key = key;
    new->value = value;
    new->bit = 0;
    return new;
}

static void __rotate_right(
    tree_node_t ** pa
)
{
    tree_node_t *child = (*pa)->left;
    assert(child);
    (*pa)->left = child->right;
    child->right = *pa;
    *pa = child;
}

static void __rotate_left(
    tree_node_t ** pa
)
{
    tree_node_t *child = (*pa)->right;
    assert(child);
    (*pa)->right = child->left;
    child->left = *pa;
    *pa = child;
}

/**
 * bring this value to the top
 * */
static tree_node_t *__splay(
    pseudolru_t * me,
    int update_if_not_found,
    tree_node_t ** gpa,
    tree_node_t ** pa,
    tree_node_t ** child,
    const void *key
)
{
    /* if no child, we have reached the bottom of the tree with no success: exit */
    if (!(*child))
    {
        return NULL;
    }

    assert(me->cmp);

    int cmp = me->cmp((*child)->key, key);

    tree_node_t *next;

    if (cmp == 0)
    {
        /* we have found the item */
        next = *child;
    }
    else if (cmp > 0)
    {
        (*child)->bit = RIGHT;
        next =
            __splay(me, update_if_not_found, pa, child, &(*child)->left, key);
    }
    else
    {
        (*child)->bit = LEFT;
        next =
            __splay(me, update_if_not_found, pa, child, &(*child)->right, key);
    }

    if (!next)
    {
        if (update_if_not_found)
        {
            next = *child;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        if (next != *child)
            return next;
    }

    if (!pa)
        return next;

    if (!gpa)
    {
        /* zig left */
        if ((*pa)->left == next)
        {
            __rotate_right(pa);
        }
        /* zig right */
        else
        {
            __rotate_left(pa);
        }
        return next;
    }

    assert(gpa);

    /* zig zig left */
    if ((*pa)->left == next && (*gpa)->left == *pa)
    {
        __rotate_right(pa);
        __rotate_right(gpa);
    }
    /* zig zig right */
    else if ((*pa)->right == next && (*gpa)->right == *pa)
    {
        __rotate_left(pa);
        __rotate_left(gpa);
    }
    /* zig zag right */
    else if ((*pa)->right == next && (*gpa)->left == *pa)
    {
        // TODO: test cases don't cover this conditional
        __rotate_left(pa);
        __rotate_right(gpa);
    }
    /* zig zag left */
    else if ((*pa)->left == next && (*gpa)->right == *pa)
    {
        __rotate_right(pa);
        __rotate_left(gpa);
    }

    return next;
}

int pseudolru_is_empty(
    pseudolru_t * me
)
{
    return NULL == me->root;
}

void *pseudolru_get(
    pseudolru_t * me,
    const void *key
)
{
    tree_node_t *node;

    node = __splay(me, 0, NULL, NULL, (tree_node_t **) & me->root, key);
    return node ? node->value : NULL;
}

void *pseudolru_remove(
    pseudolru_t * me,
    const void *key
)
{

    if (!pseudolru_get(me, key))
    {
        return NULL;
    }

    tree_node_t *root = me->root;
    void *val = root->value;

    assert(0 < me->count);
    assert(root->key == key);

    /* get left side's most higest value node */
    if (root->left)
    {
        tree_node_t *prev = root;
        tree_node_t *left_highest = root->left;

        /*  get furtherest right - since this is 'higher' */
        while (left_highest->right)
        {
            prev = left_highest;
            left_highest = left_highest->right;
        }

        /* do the swap */
        if (prev != root)
        {
            prev->right = left_highest->left;
            left_highest->left = root->left;
        }
        me->root = left_highest;
        left_highest->right = root->right;
    }
    /* there is no left */
    else
    {
        assert(root);
        me->root = root->right;
    }

    me->count--;

    free(root);

    return val;
}

static tree_node_t *__get_lru(
    tree_node_t * node
)
{
    if (!node)
    {
        return NULL;
    }

    if (node->bit == RIGHT)
    {
        if (node->right)
        {
            return __get_lru(node->right);
        }
    }
    else if (node->bit == LEFT)
    {
        if (node->left)
        {
            return __get_lru(node->left);
        }
    }

    return node;
}

void *pseudolru_pop_lru(
    pseudolru_t * dt
)
{
    return pseudolru_remove(dt, __get_lru(dt->root)->key);
}

#if 0
static int __count(
    tree_node_t * node
)
{
    if (!node)
    {
        return 0;
    }
    else
    {
        return 1 + __count(node->left) + __count(node->right);
    }
}
#endif

int pseudolru_count(
    pseudolru_t * me
)
{
#if 1
    return me->count;
#else
    return __count(me->root);
#endif
}

void *pseudolru_peek(
    pseudolru_t * me
)
{
    return me->root ? ((tree_node_t *) me->root)->value : NULL;
}

void pseudolru_put(
    pseudolru_t * me,
    void *key,
    void *value
)
{
    tree_node_t *new;

    if (!me->root)
    {
        me->root = __init_node(key, value);
        me->count++;
        goto exit;
    }

    new = __splay(me, 1, NULL, NULL, (tree_node_t **) & me->root, key);

    int cmp = me->cmp(((tree_node_t *) me->root)->key, key);

    if (cmp != 0)
    {
        new = __init_node(key, value);

        if (0 < cmp)
        {
            new->right = me->root;
            new->left = new->right->left;
            new->right->left = NULL;
        }
        else
        {
            new->left = me->root;
            new->right = new->left->right;
            new->left->right = NULL;
        }

        me->count++;
    }

    me->root = new;

  exit:
    return;
}

#if 0
static void __traverse(
    tree_node_t * node,
    int d
)
{
    if (!node)
        return;

    int ii;

    for (ii = 0; ii < d; ii++)
        printf(" ");
    printf("%lx\n", (unsigned long int) (void *) node);

    if (node->right)
    {
        for (ii = 0; ii < d; ii++)
            printf(" ");
        printf(" R");
        traverse(node->right, d + 1);
    }
    if (node->left)
    {
        for (ii = 0; ii < d; ii++)
            printf(" ");
        printf(" L");
        traverse(node->left, d + 1);
    }
}
#endif
