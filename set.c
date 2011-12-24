/*
 * =====================================================================================
 *
 *       Filename:  set.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/25/11 17:30:00
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */


/*
*  C Implementation: tea_hashmap
*
* Description: 
*
*
* Author: Willem-Hendrik Thiart <beandaddy@proxima>, (C) 2006
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include "set.h"

#define SPACERATIO 0.1  // when we call for more capacity
#define INITIAL_CAPACITY 11
#define DEBUG 0
#define DEBUGZ 0
#define CHECK 1

typedef struct hash_node_s hash_node_t;

struct hash_node_s
{
    void *value;
    hash_node_t *next;
};

static void __ensurecapacity(
    tea_hashmap_t * hmap
);

static hash_node_t *__allocnodes(
    tea_hashmap_t * hmap,
    int count
);

inline static void __freenodes(
    tea_hashmap_t * hmap,
    int count,
    hash_node_t * nodes
)
{
    free(nodes);
}


typedef int (
    *func_inthash_f
)   (
    void *
);

tea_hashmap_t *tea_hashmap_init(
    func_inthash_f hash
)
{
    tea_hashmap_t *hmap = tea_malloc_zero(sizeof(tea_hashmap_t));

    assert(hmap);
    memset(hmap, 0, sizeof(tea_hashmap_t));
    hmap->arraySize = INITIAL_CAPACITY;
    hmap->array = __allocnodes(hmap, hmap->arraySize);
    hmap->func_hash = hash;
}

int tea_hashmap_size(
    tea_hashmap_t * hmap
)
{
    return hmap->count;
}

/** frees all the nodes in a chain, recursively. */
static void __node_empty(
    tea_hashmap_t * hmap,
    hash_node_t * node
)
{
    if (NULL == node)
    {
        return;
    }
    else
    {
        __node_empty(hmap, node->next);
        free(node);
        hmap->count--;
    }
}

/** Empty this hashmap. */
void tea_hashmap_clear(
    tea_hashmap_t * hmap
)
{
    int ii;

    hash_node_t *node;

    for (ii = 0; ii < hmap->arraySize; ii++)
    {
        node = &((hash_node_t *) hmap->array)[ii];

        if (NULL == node->ety.key)
            continue;

#if DEBUGZ
        node->ety.inuse = FALSE;        // normal actions will overwrite the value
#endif
        node->ety.key = NULL;   // normal actions will overwrite the value

        /* empty and free this chain */
        __node_empty(hmap, node->next);
        node->next = NULL;

        hmap->count--;
        assert(0 <= hmap->count);

        /*hash_node_t*  old = node;
         * node = node->next;
         * while (node) {
         * hash_node_t* last = node;
         * node = node->next;
         * free(last);
         * }
         * old->next = NULL;
         */
    }

    assert(tea_hashmap_isEmpty(hmap));
//      hmap->count = 0;
}

/** Check if this hashmap is empty. */
bool tea_hashmap_isEmpty(
    tea_hashmap_t * hmap
)
{
    if (NULL == hmap)
        return TRUE;
//      assert(NULL != hmap);
    //if (NULL == hmap) return true;
    assert(0 <= hmap->count);   // greater than or equal to zero
    return 0 == hmap->count;
}


/* Free all the memory related to this hashmap. */
void tea_hashmap_free(
    tea_hashmap_t * hmap
)
{
    assert(hmap);
    tea_hashmap_clear(hmap);
    free(hmap->in);
}

/* Free all the memory related to this hashmap.
 * This includes the actual hmap itself. */
void tea_hashmap_freeall(
    tea_hashmap_t * hmap
)
{
    assert(hmap);
    tea_hashmap_free(hmap);
    free(hmap);
}

inline static uint __doProbe(
    tea_hashmap_t * hmap,
    const void *key
)
{
    /* this probably gets called a gajillion times */
    //return hmap->hashFunc(key)%hmap->arraySize;
    return hmap->obj->hash(key) % hmap->arraySize;
}

/** Get this key's value. */
void *tea_hashmap_get(
    tea_hashmap_t * hmap,
    const void *key
)
{
    int probe;

    hash_node_t *node;

    const void *key2;

    if (tea_hashmap_isEmpty(hmap))
        return NULL;

    probe = __doProbe(hmap, key);
    node = &((hash_node_t *) hmap->array)[probe];
    key2 = node->ety.key;

    if (NULL == key2)
    {
        return NULL;    /* this one wasn't assigned */
    }
    else
    {
        while (node)
        {
            key2 = node->ety.key;
            if (0 == hmap->obj->compare(key, key2))
            {
#if DEBUG
                printf("hash got %s\n", key);
#endif
                assert(node->ety.val);
                return (void *) node->ety.val;
            }
            node = node->next;
        }
    }

    return NULL;
}

/* Is this key inside this map? */
int tea_hashmap_containsKey(
    tea_hashmap_t * hmap,
    const void *key
)
{
    return (NULL != tea_hashmap_get(hmap, key));
}

#if 0
static tea_entry_t __nullentry(
)
{
    tea_entry_t entry;

    entry.key = NULL;
    entry.val = NULL;
    return entry;
}
#endif

/** Remove the value refrenced by this key from the hashmap. */
void tea_hashmap_removeEntry(
    tea_hashmap_t * hmap,
    tea_entry_t * entry,
    const void *key
)
{
    uint probe;

    hash_node_t *node;

    void *key2;

    assert(key);

//    assert(tea_hashmap_containsKey(hmap, key));
//      if (!tea_hashmap_containsKey(hmap, key)) return nullentry();

    /* it's nice if we have guaranteed success */

    probe = __doProbe(hmap, key);
    node = &((hash_node_t *) hmap->array)[probe];
    key2 = node->ety.key;

    assert(0 <= hmap->count);

    if (!key2)
    {

    }
    else if (0 == hmap->obj->compare(key, key2))
    {
        memcpy(entry, &node->ety, sizeof(tea_entry_t));

        /* if we forget about collisions we will suffer */
        /* work linked list */
        if (node->next)
        {
            hash_node_t *tmp = node->next;

            memcpy(&node->ety, &tmp->ety, sizeof(tea_entry_t));
            node->next = tmp->next;
            __freenodes(hmap, 1, tmp);
        }
        else
        {
            /* im implying that pointing towards NULL is a bottleneck */
            node->ety.key = NULL;
            node->ety.val = NULL;
#if DEBUGZ
            node->inuse = FALSE;
#endif
        }
        hmap->count--;
        return;
    }
    else
    {
        hash_node_t *node_parent = node;

        node = node->next;

        /* check chain */
        while (node)
        {
            assert(node->ety.key);
            key2 = node->ety.key;

            if (0 == hmap->obj->compare(key, key2))
            {
                memcpy(entry, &node->ety, sizeof(tea_entry_t));
                /* do a node splice */
                node_parent->next = node->next;
                free(node);
                hmap->count--;
                return;
            }

            node_parent = node;
            node = node->next;
        }
    }

    /* only gets here if it doesn't exist */
    entry->key = NULL;
    entry->val = NULL;
}

/** Remove this key and value from the map.
 * @return value of key, or NULL on failure */
void *tea_hashmap_remove(
    tea_hashmap_t * hmap,
    const void *key
)
{
    tea_entry_t entry;

    tea_hashmap_removeEntry(hmap, &entry, key);
    return (void *) entry.val;
}

inline static void __nodeassign(
    tea_hashmap_t * hmap,
    hash_node_t * node,
    void *key,
    void *val
)
{
    hmap->count++;
    assert(hmap->count < 32768);
    node->ety.key = key;
    node->ety.val = val;
#if DEBUGZ
    node->inuse = true;
#endif
}

/* Associate key with val.
 * Does not insert key if an equal key exists.
 * @return previous associated val, otherwise null */
void *tea_hashmap_put(
    tea_hashmap_t * hmap,
    void *key,
    void *val
)
{
    hash_node_t *node;

    void *key2;

    assert(key && val);
//      if (NULL == key || NULL == val) return NULL;
//      if (tea_hashmap_containsKey(hmap, key)) return;

//      printf("ADD count %d\n", hmap->count);

    __ensurecapacity(hmap);

//      probe = __doProbe(hmap,key);
    node = &((hash_node_t *) hmap->array)[__doProbe(hmap, key)];
    key2 = node->ety.key;

    /* this one wasn't assigned */
    if (NULL == key2)
    {
//              tea_msg(mSYS,0,"no collision...\n");
        __nodeassign(hmap, node, key, val);
    }
    else
    {
//              tea_msg(mSYS,0,"collision!\n");
        /* check the linked list */
        do
        {
            /* replacing val */
            if (0 == hmap->obj->compare(key, key2))
            {
                void *val_prev = node->ety.val;

                node->ety.val = val;
                return val_prev;
            }
            key2 = node->ety.key;
        }
        while (node->next && (node = node->next));
        node->next = __allocnodes(hmap, 1);
        __nodeassign(hmap, node->next, key, val);
//              printf("collide!!!\n");
    }

    return NULL;
}

void tea_hashmap_putEntry(
    tea_hashmap_t * hmap,
    tea_entry_t * entry
)
{
    tea_hashmap_put(hmap, entry->key, entry->val);
}

// FIXME: make a chain node reservoir
/** Allocate memory for nodes. used for chained nodes. */
static hash_node_t *__allocnodes(
    tea_hashmap_t * hmap,
    int count
)
{
    hash_node_t *nodes = tea_malloc_zero(sizeof(hash_node_t) * count);

//      int ii =0;
//      for (ii=0; ii<count; ii++) nodes[ii].next = NULL;
    return nodes;
}

static void __ensurecapacity(
    tea_hashmap_t * hmap
)
{
    if ((float) hmap->count / hmap->arraySize < SPACERATIO)
        return;

    int ii;

    int asize_old = hmap->arraySize;

    hash_node_t *array_old = hmap->array;       // store old array

//#if CHECK
    //int           count_old = hmap->count; // just for checking
//#endif

    hmap->arraySize *= 2;       // double array capacity
    hmap->array = __allocnodes(hmap, hmap->arraySize);
    hmap->count = 0;

    tea_msg(mSYS, 1, "enlargining hash: %d -> %d\n", asize_old,
            hmap->arraySize);

    for (ii = 0; ii < asize_old; ii++)
    {
        hash_node_t *node = &((hash_node_t *) array_old)[ii];

//              hash_node_t*    nodebase = node;

#if DEBUGZ
        if (!node->inuse)
            continue;   // if key is null,
#else
        if (NULL == node->ety.key)
            continue;   // if key is null,
#endif

        tea_hashmap_put(hmap, node->ety.key, node->ety.val);

        /* add chained hash nodes */
        //hash_node_t*  node_parent = node;
        node = node->next;

        while (node)
        {
            hash_node_t *next = node->next;

//                      if (NULL != node->ety.key) { // if key is null, skip
            tea_hashmap_put(hmap, node->ety.key, node->ety.val);
//                      }

            assert(NULL != node->ety.key);

            free(node);
            node = next;
        }

//              free(nodebase);
        //node_parent->next = NULL;
    }

#if CHECK
/*
	if (hmap->count!=count_old) {
		tea_msg(mSYS,0,"ERROR: hashmap is corrupt, has outside modified entries: %d vs %d\n",
			 hmap->count, count_old); 
		tea_msg(mSYS,0,"ERROR: hashmap has %d duplicate%c\n",
			count_old - hmap->count, count_old - hmap->count == 1 ? ' ' : 's'); 
	}
	assert(hmap->count==count_old);
*/
//      tea.x86: src/adt/tea_hashmap_linked.c:366: ensurecapacity: Assertion `hmap->count==count_old' failed. 060130
#endif

    free(array_old);
}

/*
 * ==============================
 *  || | | | | | ITERATOR | | ||
 * ==============================
 */

typedef struct
{
    hash_node_t *linkedCur;     /* current linked slot being checked */
    void *dt;
} hashmap_iter_t;

static int __has_next(
    void *data
)
{
    tea_iter_t *iter = data;

    hashmap_iter_t *map_iter = iter->data;

    tea_hashmap_t *hmap;

    hmap = map_iter->dt;

    if (NULL == map_iter->linkedCur)
    {
        for (; iter->current < hmap->arraySize; iter->current++)
        {
            hash_node_t *node;

            node = &((hash_node_t *) hmap->array)[iter->current];
//          if (node->ety.value((void**)hmap->array)[iter->current]) break;
            if (node->ety.val)
                break;
        }

//      printf("checking: %d of %d\n", iter->current, hmap->arraySize);

        if (iter->current == hmap->arraySize)
        {
            return 0;
        }
    }

    return 1;
}

static void *__peek_val(
    void *data
)
{
    hashmap_iter_t *map_iter = tea_iter(data)->data;

    if (!__has_next(data))
    {
        return NULL;
    }
    else if (map_iter->linkedCur)
    {
        return map_iter->linkedCur->ety.val;
    }
    else
    {
        hash_node_t *node;

        tea_hashmap_t *hmap = map_iter->dt;

        node = &((hash_node_t *) hmap->array)[tea_iter(data)->current];

        return node->ety.val;
    }
}

static void *__next_val(
    void *data
)
{
    tea_iter_t *iter = data;

    hashmap_iter_t *map_iter = iter->data;

    tea_hashmap_t *hmap;

    hash_node_t *node;

    hmap = map_iter->dt;

    if (!__has_next(iter))
    {
        return NULL;
    }

//      if (iter->current == hmap->arraySize) return NULL;
    if (map_iter->linkedCur)
    {
        void *ret;

        ret = map_iter->linkedCur->ety.val;
        map_iter->linkedCur = map_iter->linkedCur->next;
        return ret;
    }
    else
    {
        node = &((hash_node_t *) hmap->array)[iter->current];

        if (node->next)
        {
            map_iter->linkedCur = node->next;
        }

        iter->current++;
        return node->ety.val;
    }
}

static void *__next_key(
    void *data
)
{
    tea_iter_t *iter = data;

    hashmap_iter_t *map_iter = iter->data;

    tea_hashmap_t *hmap;

    hmap = map_iter->dt;

    if (!__has_next(iter))
        return NULL;

    if (map_iter->linkedCur)
    {
        void *ret;

        FIXME_NEEDS_WORK;
        ret = map_iter->linkedCur->ety.key;

        map_iter->linkedCur = map_iter->linkedCur->next;
        return ret;
    }

    hash_node_t *node = &((hash_node_t *) hmap->array)[iter->current];

    if (node->next)
    {
        map_iter->linkedCur = node->next;
    }
    else
    {

    }

    iter->current++;
    return node->ety.key;
}

static void __done(
    void *data
)
{
    tea_iter_t *iter = data;

//      hashmap_iter_t* map_iter = iter->data;
    tea_pool_free(iter->data);
    tea_pool_free(data);
}

tea_iter_t *tea_hashmap_iter(
    tea_hashmap_t * dt
)
{
    tea_iter_t *iter;

    hashmap_iter_t *map_iter;

    iter = tea_pool_malloc_zero(sizeof(tea_iter_t));
    map_iter = tea_pool_malloc_zero(sizeof(hashmap_iter_t));
    map_iter->dt = dt;
    iter->data = map_iter;
    iter->done = __done;
    iter->next = __next_val;
    iter->has_next = __has_next;
    iter->peek = __peek_val;
    iter->current = 0;
    return iter;
}

tea_iter_t *tea_hashmap_iterKeys(
    tea_hashmap_t * dt
)
{
    tea_iter_t *iter = tea_hashmap_iter(dt);

    iter->next = __next_key;
    iter->next_const = __next_key;
    return iter;
}

/*--------------------------------------------------------------79-characters-*/
