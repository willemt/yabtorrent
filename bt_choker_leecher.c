/**
 * @file
 * @brief This is the choker ruleset when the client is leeching
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

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "block.h"
#include "bt.h"
#include "bt_local.h"

#include "linked_list_queue.h"
#include "linked_list_hashmap.h"
#include "heap.h"

typedef struct
{
    /* the most number of unchoked peers we can have */
    int max_unchoked_peers;
    /*  last time we checked who was choked */
    int time_last_choke_check;
    hashmap_t *peers;
    
    /*  the head node is the node that will be choked */
    /*  nodes that are choked are removed from the queue */
    linked_list_queue_t *peers_unchoked;
    linked_list_queue_t *peers_choked;
    linked_list_queue_t *peers_waiting_for_optimistic_unchoke;
    
    /* choker peer interface, for callbacks on the peer */
    bt_choker_peer_i *iface;
    
    void *udata;
} choker_t;

/*----------------------------------------------------------------------------*/

static unsigned long __peer_hash(const void *obj)
{
    return (unsigned long) obj;
}

static long __peer_compare(const void *obj, const void *other)
{
    return obj - other;
}

/*----------------------------------------------------------------------------*/

/**
 * Create a new leeching choker
 * @param size: the maximum number of unchoked peers
 */
void *bt_leeching_choker_new(const int size)
{
    choker_t *ch;

    ch = calloc(1, sizeof(choker_t));
    ch->max_unchoked_peers = size;
    ch->peers = hashmap_new(__peer_hash, __peer_compare, 11);
    ch->peers_unchoked = llqueue_new();
    ch->peers_choked = llqueue_new();
    ch->peers_waiting_for_optimistic_unchoke = llqueue_new();
    return ch;
}

/**
 * Start managing a new peer */
void bt_leeching_choker_add_peer(void *ckr, void *peer)
{
    choker_t *ch = ckr;
    
    /* Don't add the same peer again */
    if (hashmap_contains_key(ch->peers, peer))
    {
      return;
    }

    hashmap_put(ch->peers, peer, peer);
    llqueue_offer(ch->peers_choked, peer);
    llqueue_offer(ch->peers_waiting_for_optimistic_unchoke, peer);
}

/**
 * Stop managing this peer */
void bt_leeching_choker_remove_peer(void *ckr, void *peer)
{
    choker_t *ch = ckr;

    hashmap_remove(ch->peers, peer);
}

/*----------------------------------------------------------------------------*/

static void __choke_peer(choker_t * ch, void *peer)
{
    llqueue_remove_item(ch->peers_unchoked, peer);
    /*  we're back in the queue for being allowed back */
    llqueue_offer(ch->peers_choked, peer);
    llqueue_offer(ch->peers_waiting_for_optimistic_unchoke, peer);
    ch->iface->choke_peer(ch->udata, peer);
}

void bt_leeching_choker_announce_interested_peer(void *cho, void *peer)
{
 // @TODO
}

/*----------------------------------------------------------------------------*/

/** 
 * function used in heap for priority 
 * */
static int __cmp_peer_priority_datarate(const void *i1,
                                        const void *i2, const void *ckr)
{
    const choker_t *ch = ckr;

#if 0
    const peer_t *p1 = i1;
    const peer_t *p2 = i2;

    return ch->iface->get_urate(ch->udata,
                                p1->udata_peer) -
        ch->iface->get_urate(ch->udata, p2->udata_peer);
#else
    const void *p1 = i1;
    const void *p2 = i2;

    assert(ch->iface);
    assert(ch->iface->get_urate);

    return ch->iface->get_urate(ch->udata, p1) -
        ch->iface->get_urate(ch->udata, p2);
#endif
}

static int __cmp_peer_priority_datarate_inverse(const void *i1,
                                                const void *i2, const void *ckr)
{
    return -__cmp_peer_priority_datarate(i1, i2, ckr);
}

/*----------------------------------------------------------------------------*/

void bt_leeching_choker_decide_best_npeers(void *ckr)
{
    choker_t *ch = ckr;

    heap_t *hp;

    int ii;

    hp = heap_new(__cmp_peer_priority_datarate, ckr);

    /*  poll all peers and offer to priority queue */

    while (0 < llqueue_count(ch->peers_unchoked))
    {
        heap_offer(hp, llqueue_poll(ch->peers_unchoked));
    }

    while (0 < llqueue_count(ch->peers_choked))
    {
        heap_offer(hp, llqueue_poll(ch->peers_choked));
    }

    /*  poll best four from priority queue */

    for (ii = 0; ii < ch->max_unchoked_peers; ii++)
    {
        void *peer;

        peer = heap_poll(hp);

        bt_leeching_choker_unchoke_peer(ckr, peer);
    }

    /*  empty residual peers into choked bucket */

    while (0 < heap_count(hp))
    {
        __choke_peer(ch, heap_poll(hp));
    }

    heap_free(hp);
}

static void __choke_worst_downloader(choker_t * ch)
{
    heap_t *hp;

    int ii;

    hp = heap_new(__cmp_peer_priority_datarate_inverse, ch);

    /*  poll downloading peers and offer to priority queue */

    while (0 < llqueue_count(ch->peers_unchoked))
    {
        heap_offer(hp, llqueue_poll(ch->peers_unchoked));
    }

    /*  poll best four from priority queue */

    for (ii = 0; ii < 1; ii++)
    {
        void *peer;

        peer = heap_poll(hp);
        __choke_peer(ch, peer);
    }

    /*  empty residual peers into choked bucket */

    while (0 < heap_count(hp))
    {
        llqueue_offer(ch->peers_unchoked, heap_poll(hp));
    }

    heap_free(hp);
}

void bt_leeching_choker_optimistically_unchoke(void *ckr)
{
    choker_t *ch = ckr;
    int ii, end;
    void *peer;

    /* go through peers waiting to be optimistically unchoked... */
    for (ii = 0, end = llqueue_count(ch->peers_waiting_for_optimistic_unchoke);
         ii < end; ii++)
    {
        peer = llqueue_poll(ch->peers_waiting_for_optimistic_unchoke);

        /* ...if the peer is interested... */
        if (1 == ch->iface->get_is_interested(ch->udata, peer))
        {
            __choke_worst_downloader(ch);
            bt_leeching_choker_unchoke_peer(ch, peer);
            break;
        }

        /* ...otherwise, better luck next time */
        llqueue_offer(ch->peers_waiting_for_optimistic_unchoke, peer);
    }
}

void bt_leeching_choker_unchoke_peer(void *ckr, void *peer)
{
    choker_t *ch = ckr;

    void *pr;

    assert(hashmap_contains_key(ch->peers, peer));
    
    ch->iface->unchoke_peer(ch->udata, peer);
    pr = hashmap_get(ch->peers, peer);
    /*  ensure that this peer is on the tail of the queue */
    llqueue_remove_item(ch->peers_choked, peer);
    llqueue_remove_item(ch->peers_unchoked, peer);
    llqueue_remove_item(ch->peers_waiting_for_optimistic_unchoke, peer);
    llqueue_offer(ch->peers_unchoked, peer);
}

/**
 * Get number of peers */
int bt_leeching_choker_get_npeers(void *ckr)
{
    choker_t *ch = ckr;

    return hashmap_count(ch->peers);
}

void bt_leeching_choker_set_choker_peer_iface(void *ckr,
                                              void *udata,
                                              bt_choker_peer_i * iface)
{
    choker_t *ch = ckr;

    ch->udata = udata;
    ch->iface = iface;
}

/**
 * Get choker interface
 */
void bt_leeching_choker_get_iface(bt_choker_i * iface)
{
    iface->new = bt_leeching_choker_new;
    iface->unchoke_peer = bt_leeching_choker_unchoke_peer;
    iface->add_peer = bt_leeching_choker_add_peer;
    iface->remove_peer = bt_leeching_choker_remove_peer;
    iface->get_npeers = bt_leeching_choker_get_npeers;
    iface->set_choker_peer_iface = bt_leeching_choker_set_choker_peer_iface;
    iface->decide_best_npeers = bt_leeching_choker_decide_best_npeers;
}
