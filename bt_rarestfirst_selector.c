/*
 * =====================================================================================
 *
 *       Filename:  bt_rarestfirst_selector.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/29/11 23:39:01
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include "bt.h"
#include "bt_local.h"

#include "linked_list_queue.h"
#include "linked_list_hashmap.h"
#include "heap.h"

/*  rarestfirst  */
typedef struct
{
    hashmap_t *peers;
//    heap_t *piece_heap;
    hashmap_t *pieces;
    /*  pieces that we've polled */
    hashmap_t *pieces_polled;
    int npieces;
} rarestfirst_t;

/*  peer */
typedef struct
{
    hashmap_t *have_pieces;
//    linked_list_queue_t *have_pieces;
} peer_t;

/*  piece */
typedef struct
{
    int nhaves;
    int idx;
} piece_t;

/*----------------------------------------------------------------------------*/
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
//    choker_t *ch = ckr;

    const piece_t *p1 = i1;

    const piece_t *p2 = i2;

    return p2->nhaves - p1->nhaves;
//    return ch->iface->get_urate(p1) - ch->iface->get_urate(p2);
}

/*----------------------------------------------------------------------------*/

void *bt_rarestfirst_selector_new(
    const int npieces
)
{
    rarestfirst_t *rf;

    rf = calloc(1, sizeof(rarestfirst_t));
    rf->npieces = npieces;
    rf->peers = hashmap_new(__peer_hash, __peer_compare);
    rf->pieces = hashmap_new(__peer_hash, __peer_compare);
    rf->pieces_polled = hashmap_new(__peer_hash, __peer_compare);
//    rf->piece_heap = heap_new(__cmp_piece, rf);
//    = llqueue_new();
    return rf;
}

void bt_rarestfirst_selector_free(
    void *r
)
{
    rarestfirst_t *rf = r;

    hashmap_free(rf->peers);
    hashmap_free(rf->pieces);
    hashmap_free(rf->pieces_polled);
//    heap_free(rf->piece_heap);
    free(rf);
}

/**
 * Add this piece back to the selector */
void bt_rarestfirst_selector_offer_piece(
    void *r,
    int piece_idx
)
{
    rarestfirst_t *rf = r;

    piece_t *pce;

    pce = hashmap_remove(rf->pieces_polled, (void *) (long) piece_idx);

    if (pce)
    {
        hashmap_put(rf->pieces, (void *) (long) piece_idx, pce);
    }
}

void bt_rarestfirst_selector_announce_have_piece(
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

void bt_rarestfirst_selector_remove_peer(
    void *r,
    void *peer
)
{
    rarestfirst_t *rf = r;

    peer_t *pr;

    /*  get the peer */
    pr = hashmap_remove(rf->peers, peer);

    if (pr)
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

    /*  get the peer */
    pr = hashmap_get(rf->peers, peer);

    if (!pr)
    {
        pr = malloc(sizeof(peer_t));
        pr->have_pieces = hashmap_new(__peer_hash, __peer_compare);
        hashmap_put(rf->peers, peer, pr);
    }
}

/**
 * Let us know that there is a peer who has this piece
 */
void bt_rarestfirst_selector_announce_peer_have_piece(
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

    piece = hashmap_get(rf->pieces, (void *) (long) piece_idx);

    if (!piece)
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

int bt_rarestfirst_selector_get_npeers(
    void *r
)
{
    rarestfirst_t *rf = r;

    return hashmap_count(rf->peers);
}

int bt_rarestfirst_selector_get_npieces(
    void *r
)
{
    rarestfirst_t *rf = r;

    return rf->npieces;
}

/**
 * poll best piece from peer */
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

    pr = hashmap_get(rf->peers, peer);

    if (!pr)
    {
        return -1;
    }

    /*  if we have no pieces, fail */
    if (0 == hashmap_count(pr->have_pieces))
    {
        return -1;
    }

    hp = heap_new(__cmp_piece, rf);

    /*  look at master hashmap */
    hashmap_iterator(rf->pieces, &iter);

    /*  add to priority queue */
    while ((p = hashmap_iterator_next(rf->pieces, &iter)))
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
