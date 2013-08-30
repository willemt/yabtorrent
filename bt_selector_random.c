
/**
 * @file
 * @brief Select a random piece to download
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "block.h"
#include "bt.h"
#include "bt_local.h"

#include "linked_list_queue.h"
#include "linked_list_hashmap.h"
#include "bag.h"
#include "heap.h"

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
    rf->peers = hashmap_new(__peer_hash, __peer_compare, 11);
    rf->p_polled = hashmap_new(__peer_hash, __peer_compare, 11);
    return rf;
}

void bt_random_selector_free(
    void *r
)
{
    random_t *rf = r;

    hashmap_free(rf->peers);
//    bag_free(rf->p_candidates);
    hashmap_free(rf->p_polled);
    free(rf);
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

/**
 * Add this piece back to the selector.
 * This is usually when we want to make the piece a candidate again*/
void bt_random_selector_offer_piece(
    void *r,
    int piece_idx
)
{

}

/**
 * Notify selector that we have this piece */
void bt_random_selector_announce_have_piece(
    void *r,
    int piece_idx
)
{
    random_t *rf = r;

    hashmap_put(rf->p_polled, (void *) (long) piece_idx, (void *) (long) piece_idx);
}

/**
 * Let us know that there is a peer who has this piece
 */
void bt_random_selector_announce_peer_have_piece(
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

    if (!(p = hashmap_get(rf->p_polled, (void *) (long) piece_idx)))
    {
        bag_put(pr->p_candidates, (void *) (long) piece_idx);
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

/**
 * Poll best piece from peer
 * @param r random object
 * @param peer Best piece in context of this peer
 * @return idx of piece which is best; otherwise -1 */
int bt_random_selector_poll_best_piece(
    void *r,
    const void *peer
)
{
    random_t *rf = r;
    heap_t *hp;
    peer_t *pr;
    int piece_idx;

    if (!(pr = hashmap_get(rf->peers, peer)))
    {
        return -1;
    }

    while (0 < bag_count(pr->p_candidates))
    {
        piece_idx = (int)bag_take(pr->p_candidates);

        if (!(hashmap_get(rf->p_polled, (void *) (long) piece_idx)))
        {
            hashmap_put(rf->p_polled, (void *) (long) piece_idx, (void *) (long) piece_idx);
            return piece_idx;
        }
    }

    return -1;
}

