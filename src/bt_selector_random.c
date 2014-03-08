
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Select a random piece to download
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
#include "bag.h"

/*  random  */
typedef struct
{
    hashmap_t *peers;

    /*  pieces that are candidates for polling  */
    //hashmap_t *p_candidates;

    /*  pieces that we've polled */
    hashmap_t *p_polled;

    /*  number of pieces to download */
    int npieces;
} random_t;

/*  peer */
typedef struct
{
    //hashmap_t *have_pieces;
    bag_t *p_candidates;
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

void *bt_random_selector_new(
    const int npieces
)
{
    random_t *rf;

    rf = calloc(1, sizeof(random_t));
    rf->npieces = npieces;
    rf->peers = hashmap_new(__peer_hash, __peer_compare, 17);
    rf->p_polled = hashmap_new(__peer_hash, __peer_compare, 17);
    return rf;
}

void bt_random_selector_free(
    void *r
)
{
    random_t *rf = r;

//    hashmap_free(rf->peers);
//    bag_free(rf->p_candidates);
//    hashmap_free(rf->p_polled);
//    free(rf);
}

void bt_random_selector_remove_peer(
    void *r,
    void *peer
)
{
    random_t *rf = r;
    peer_t *pr;

    if ((pr = hashmap_remove(rf->peers, peer)))
    {
//        hashmap_free(pr->have_pieces);
        free(pr);
    }
}

void bt_random_selector_add_peer(
    void *r,
    void *peer
)
{
    random_t *rf = r;
    peer_t *pr;

    /* make sure not to add duplicates */
    if ((pr = hashmap_get(rf->peers, peer)))
        return;

    pr = calloc(1,sizeof(peer_t));
    pr->p_candidates = bag_new();
//    pr->have_pieces = hashmap_new(__peer_hash, __peer_compare, 11);
    hashmap_put(rf->peers, peer, pr);
}

void bt_random_selector_giveback_piece(
    void *r,
    void* peer,
    int piece_idx
)
{
    random_t *rf = r;
    peer_t *pr;
    void* p;

    hashmap_remove(rf->p_polled, (void *) (long) piece_idx + 1);

    if (peer)
    {
        pr = hashmap_get(rf->peers, peer);
        assert(pr);
        bag_put(pr->p_candidates, (void *) (long) piece_idx + 1);
    }
}

void bt_random_selector_have_piece(
    void *r,
    int piece_idx
)
{
    random_t *rf = r;

    assert(rf);
    assert(rf->p_polled);
    hashmap_put(rf->p_polled, (void *) (long) piece_idx + 1, (void *) (long) piece_idx + 1);
}

void bt_random_selector_peer_have_piece(
    void *r,
    void *peer,
    const int piece_idx
)
{
    random_t *rf = r;
    peer_t *pr;
    void* p;

    /*  get the peer */
    pr = hashmap_get(rf->peers, peer);

    assert(pr);

    if (!(p = hashmap_get(rf->p_polled, (void *) (long) piece_idx + 1)))
    {
        bag_put(pr->p_candidates, (void *) (long) piece_idx + 1);
    }
}

int bt_random_selector_get_npeers(void *r)
{
    random_t *rf = r;
    return hashmap_count(rf->peers);
}

int bt_random_selector_get_npieces(void *r)
{
    random_t *rf = r;
    return rf->npieces;
}

int bt_random_selector_poll_best_piece(
    void *r,
    const void *peer
)
{
    random_t *rf = r;
    peer_t *pr;
    int piece_idx;

    if (!(pr = hashmap_get(rf->peers, peer)))
    {
        return -1;
    }

    /* get a random piece that the client might have */
    while (0 < bag_count(pr->p_candidates))
    {
        piece_idx = ((unsigned long int)bag_take(pr->p_candidates)) - 1;

        /* don't poll if it has been polled already */
        if (!(hashmap_get(rf->p_polled, (void *) (long) piece_idx + 1)))
        {
            void* i = (void *) ((long) piece_idx + 1);
            hashmap_put(rf->p_polled, i, i);
            return piece_idx;
        }
    }

    return -1;
}

