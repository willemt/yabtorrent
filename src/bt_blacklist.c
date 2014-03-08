
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief We want to block peers from giving us certain pieces.
 * @desc Because peers might have invalid data (maliciously or accidentally)
 *       we need to be prepared to block these peers from giving us pieces
 *       that have become invalidated. Information from this module can be
 *       used to idenfity peers who have been blocked multiple times.
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bt.h"
#include "bt_blacklist.h"

#include "linked_list_queue.h"
#include "linked_list_hashmap.h"
#include "bag.h"
#include "heap.h"
#include "avl_tree.h"

typedef struct
{
    /* blacklist peers from downloading this piece
     * Using AVL tree because:
     *  1. space efficiency - This is an array based AVL. Number of items is
     *                        expected to be 1-5 peers. Hashmap would be 
     *                        too sparse.
     *  2. read efficiency  - AVL is optimised for reads, writes are uncommon
     *                        as piece invalidation events are rare */
    avltree_t* peers_blacklisted;

    linked_list_queue_t* peers_potential_invalid;

    /* peers that were involved in providing blocks to this piece */
} piece_t;

typedef struct
{
    avltree_t* pieces;
} blacklist_t;

static long __cmp_piece(
    const void *i1,
    const void *i2
)
{
    return (unsigned long)i2 - (unsigned long)i1;
}

static long __cmp_address(
    const void *e1,
    const void *e2
)
{
    return (unsigned long)e2 - (unsigned long)e1;
}

void *bt_blacklist_new()
{
    blacklist_t* me;

    me = calloc(1, sizeof(blacklist_t));
    me->pieces = avltree_new(__cmp_piece);
    return me;
}

static piece_t* __init_piece()
{
    piece_t* p;

    p = malloc(sizeof(piece_t));
    p->peers_blacklisted = avltree_new(__cmp_address);
    p->peers_potential_invalid = llqueue_new();
    return p;
}

int bt_blacklist_get_npieces(void* blacklist)
{
    blacklist_t* me = blacklist;
    return avltree_count(me->pieces);
}

void bt_blacklist_add_peer(
    void* blacklist,
    void* piece,
    void* peer)
{
    blacklist_t* me = blacklist;
    piece_t* p;

    if (!(p = avltree_get(me->pieces,piece)))
    {
        p = __init_piece();
        avltree_insert(me->pieces,piece,p);
    }
    avltree_insert(p->peers_blacklisted,peer,peer);
}

void bt_blacklist_add_peer_as_potentially_blacklisted(
    void* blacklist,
    void* piece,
    void* peer)
{
    blacklist_t* me = blacklist;
    piece_t* p;

    if (!(p = avltree_get(me->pieces,piece)))
    {
        p = __init_piece();
        avltree_insert(me->pieces,piece,p);
    }
    llqueue_offer(p->peers_potential_invalid, peer);
}

int bt_blacklist_peer_is_blacklisted(
    void* blacklist,
    void* piece,
    void* peer)
{
    blacklist_t* me = blacklist;
    piece_t* p;

    if (!peer)
        return 0;

    if (!(p = avltree_get(me->pieces,piece)))
    {
        return 0;
    }

    return NULL != avltree_get(p->peers_blacklisted,peer);
}

int bt_blacklist_peer_is_potentially_blacklisted(
    void* blacklist,
    void* piece,
    void* peer)
{
    blacklist_t* me = blacklist;
    piece_t* p;

    if (!peer)
        return 0;

    if (!(p = avltree_get(me->pieces,piece)))
    {
        return 0;
    }

    return NULL != llqueue_get_item_via_cmpfunction(p->peers_potential_invalid,
            peer,  __cmp_address);
}

