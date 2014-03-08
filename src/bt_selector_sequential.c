
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Select a sequential piece to download
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bt.h"

#include "linked_list_queue.h"
#include "linked_list_hashmap.h"
#include "heap.h"

/*  sequential  */
typedef struct
{
    hashmap_t *peers;

    /*  pieces that we've polled */
    hashmap_t *p_polled;

    int npieces;

} sequential_t;

/*  peer */
typedef struct
{
    //hashmap_t *have_pieces;
    heap_t* p_candidates;
} peer_t;

static unsigned long __peer_hash(
    const void *obj
)
{
    return (unsigned long) obj;
}

static long __peer_compare(
    const void *obj,
    const void *other
)
{
    return obj - other;
}

static int __cmp_piece(
    const void *i1,
    const void *i2,
    const void *ckr
)
{
    return i2 - i1;
}

void *bt_sequential_selector_new(
    const int npieces
)
{
    sequential_t *me;

    me = calloc(1, sizeof(sequential_t));
    me->npieces = npieces;
    me->peers = hashmap_new(__peer_hash, __peer_compare, 11);
    me->p_polled = hashmap_new(__peer_hash, __peer_compare, 11);
    return me;
}

void bt_sequential_selector_free(
    void *r
)
{
    sequential_t *me = r;

    hashmap_free(me->peers);
//    heap_free(me->p_candidates);
    hashmap_free(me->p_polled);
    free(me);
}

void bt_sequential_selector_remove_peer(
    void *r,
    void *peer
)
{
    sequential_t *me = r;
    peer_t *pr;

    if ((pr = hashmap_remove(me->peers, peer)))
    {
//        hashmap_free(pr->have_pieces);
        free(pr);
    }
}

void bt_sequential_selector_add_peer(
    void *r,
    void *peer
)
{
    sequential_t *me = r;
    peer_t *pr;

    /* make sure not to add duplicates */
    if ((pr = hashmap_get(me->peers, peer)))
        return;

    pr = calloc(1,sizeof(peer_t));
    pr->p_candidates = heap_new(__cmp_piece, NULL);
    hashmap_put(me->peers, peer, pr);
}

void bt_sequential_selector_giveback_piece(
    void *r,
    void *peer,
    int piece_idx
)
{
    sequential_t *me = r;
    peer_t *pr;
    void* p;

    hashmap_remove(me->p_polled, (void *) (long) piece_idx + 1);

    if (peer)
    {
        pr = hashmap_get(me->peers, peer);
        assert(pr);
        heap_offer(pr->p_candidates, (void *) (long) piece_idx + 1);
    }

#if 0
    if (!(p = hashmap_get(me->p_polled, (void *) (long) piece_idx + 1)))
    {
        heap_offer(pr->p_candidates, (void *) (long) piece_idx + 1);
    }
#endif
}

void bt_sequential_selector_have_piece(
    void *r,
    int piece_idx
)
{
    sequential_t *me = r;

    assert(me->p_polled);
    hashmap_put(me->p_polled, (void *) (long) piece_idx + 1, (void *) (long) piece_idx + 1);
}

void bt_sequential_selector_peer_have_piece(
    void *r,
    void *peer,
    const int piece_idx
)
{
    sequential_t *me = r;
    peer_t *pr;
    void* p;

    /*  get the peer */
    pr = hashmap_get(me->peers, peer);

    assert(pr);

    if (!(p = hashmap_get(me->p_polled, (void *) (long) piece_idx + 1)))
    {
        heap_offer(pr->p_candidates, (void *) (long) piece_idx + 1);
    }
}

int bt_sequential_selector_get_npeers(void *r)
{
    sequential_t *me = r;

    return hashmap_count(me->peers);
}

int bt_sequential_selector_get_npieces(void *r)
{
    sequential_t *me = r;

    return me->npieces;
}

int bt_sequential_selector_poll_best_piece(
    void *r,
    const void *peer
)
{
    sequential_t *me = r;
    heap_t *hp;
    peer_t *pr;
    int piece_idx;

    if (!(pr = hashmap_get(me->peers, peer)))
    {
        return -1;
    }

    while (0 < heap_count(pr->p_candidates))
    {
        piece_idx = (unsigned long int)heap_poll(pr->p_candidates) - 1;

        if (!(hashmap_get(me->p_polled, (void *) (long) piece_idx + 1)))
        {
            hashmap_put(me->p_polled,
                    (void *) (long) piece_idx + 1, (void *) (long) piece_idx + 1);
            return piece_idx;
        }
    }

    return -1;
}

