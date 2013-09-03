
/**
 * @file
 * @brief Select the most rarest piece to download
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
#include "heap.h"

/*  rarestfirst  */
typedef struct
{
    hashmap_t *peers;
    hashmap_t *pieces;
    /*  pieces that we've polled */
    hashmap_t *pieces_polled;
    int npieces;
} rarestfirst_t;

/*  peer */
typedef struct
{
    /*  the pieces that the peer has */
    hashmap_t *have_pieces;
} peer_t;

/*  piece */
typedef struct
{
    /*  Number of peers that has this piece
     *  This is for determining rarity */
    int nhaves;
    /*  Piece IDX */
    int idx;
} piece_t;

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
    const piece_t *p1 = i1;
    const piece_t *p2 = i2;

    return p2->nhaves - p1->nhaves;
}

void *bt_rarestfirst_selector_new(
    const int npieces
)
{
    rarestfirst_t *rf;

    rf = calloc(1, sizeof(rarestfirst_t));
    rf->npieces = npieces;
    rf->peers = hashmap_new(__peer_hash, __peer_compare, 11);
    rf->pieces = hashmap_new(__peer_hash, __peer_compare, 11);
    rf->pieces_polled = hashmap_new(__peer_hash, __peer_compare, 11);
    return rf;
}

static void __peer_release(peer_t* p)
{
    hashmap_iterator_t iter;
    piece_t* pce;

    for (hashmap_iterator(p->have_pieces, &iter);
        (pce = hashmap_iterator_next(p->have_pieces, &iter));)
    {
        hashmap_remove(p->have_pieces, p);
    }
}

void bt_rarestfirst_selector_free(
    void *r
)
{
    rarestfirst_t *rf = r;
    hashmap_iterator_t iter;
    peer_t* p;

    for (hashmap_iterator(rf->pieces, &iter);
        (p = hashmap_iterator_next(rf->pieces, &iter));)
    {
        hashmap_remove(rf->pieces, p);
        free(p);
    }

    hashmap_free(rf->peers);
    hashmap_free(rf->pieces);
    hashmap_free(rf->pieces_polled);
    free(rf);
}

void bt_rarestfirst_selector_remove_peer(
    void *r,
    void *peer
)
{
    rarestfirst_t *rf = r;
    peer_t *pr;

    if ((pr = hashmap_remove(rf->peers, peer)))
    {
        hashmap_free(pr->have_pieces);
        free(pr);
    }
}

void bt_rarestfirst_selector_add_peer(
    void *r,
    void *peer
)
{
    rarestfirst_t *rf = r;
    peer_t *pr;

    if (!(pr = hashmap_get(rf->peers, peer)))
    {
        pr = calloc(1,sizeof(peer_t));
        pr->have_pieces = hashmap_new(__peer_hash, __peer_compare, 11);
        hashmap_put(rf->peers, peer, pr);
    }
}

/**
 * Add this piece back to the selector */
void bt_rarestfirst_selector_giveback_piece(
    void *r,
    void* peer,
    int piece_idx
)
{
    rarestfirst_t *rf = r;

    piece_t *pce;

    if ((pce = hashmap_remove(rf->pieces_polled, (void *) (long) piece_idx)))
    {
        hashmap_put(rf->pieces, (void *) (long) piece_idx, pce);
    }
}

void bt_rarestfirst_selector_have_piece(
    void *r,
    int piece_idx
)
{
    rarestfirst_t *rf = r;
    piece_t *pce;

    pce = hashmap_remove(rf->pieces, (void *) (long) piece_idx);
    pce = hashmap_put(rf->pieces_polled, (void *) (long) piece_idx, pce);
    /*  possible memory leak here */
}

/**
 * Let us know that there is a peer who has this piece
 */
void bt_rarestfirst_selector_peer_have_piece(
    void *r,
    void *peer,
    const int piece_idx
)
{
    rarestfirst_t *rf = r;
    piece_t *piece;
    peer_t *pr;

    /*  get the peer */
    pr = hashmap_get(rf->peers, peer);

    assert(pr);

    if (!(piece = hashmap_get(rf->pieces, (void *) (long) piece_idx)))
    {
        piece = malloc(sizeof(piece_t));
        piece->nhaves = 1;
        piece->idx = piece_idx;
        hashmap_put(rf->pieces, (void *) (long) piece_idx, piece);
    }

    /*  increment haves  */
    piece->nhaves += 1;

    /*  add to peer's pieces */
    hashmap_put(pr->have_pieces, (void *) (long) piece_idx, piece);
}

int bt_rarestfirst_selector_get_npeers(void *r)
{
    rarestfirst_t *rf = r;

    return hashmap_count(rf->peers);
}

int bt_rarestfirst_selector_get_npieces(void *r)
{
    rarestfirst_t *rf = r;

    return rf->npieces;
}

/**
 * Poll best piece from peer,
 * @param r Rarestfirst object
 * @param peer Best piece in context of this peer
 * @return idx of piece which is best; otherwise -1 */
int bt_rarestfirst_selector_poll_best_piece(
    void *r,
    const void *peer
)
{
    rarestfirst_t *rf = r;
    heap_t *hp;
    peer_t *pr;
    void *p;
    int piece_idx;
    hashmap_iterator_t iter;
    piece_t *pce;

    if (!(pr = hashmap_get(rf->peers, peer)))
    {
        return -1;
    }

    /*  if we have no pieces, fail */
    if (0 == hashmap_count(pr->have_pieces))
    {
        return -1;
    }

    hp = heap_new(__cmp_piece, rf);

    /*  add to priority queue */
    for (hashmap_iterator(rf->pieces, &iter);
        (p = hashmap_iterator_next(rf->pieces, &iter));)
    {
        /* only add if peer has it */
        if (hashmap_get(pr->have_pieces, p))
        {
            pce = hashmap_get(rf->pieces, p);
            heap_offer(hp, pce);
        }
    }

    /*  queue best from priority queue */
    if ((pce = heap_poll(hp)))
    {
        piece_idx = pce->idx;
        hashmap_remove(rf->pieces, (void *) (long) pce->idx);
        hashmap_put(rf->pieces_polled, (void *) (long) pce->idx, pce);
    }
    else
    {
        piece_idx = -1;
    }

    heap_free(hp);
    return piece_idx;
}
