/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief This is the choker ruleset when the client is seeding
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include "block.h"
#include "bt.h"
#include "bt_local.h"
#include "bt_choker_peer.h"
#include "bt_choker.h"
#include "linked_list_queue.h"
#include "linked_list_hashmap.h"

typedef struct
{
    void *udata_peer;
    /*  this is id of the unchoke operation */
//    int unchoke_id;
} peer_t;

typedef struct
{
    /* the most number of unchoked peers we can have */
    int max_unchoked_peers;
    /*  last time we checked who was choked */
    int time_last_choke_check;
    hashmap_t *peers;
    bt_choker_peer_i *iface;
    /*  the head node is the node that will be choked */
    /*  nodes that are choked are removed from the queue */
    linked_list_queue_t *peers_unchoked;
    linked_list_queue_t *peers_choked;
    /*  the current position we are at on the unchoke id - sequential */
//    int unchoke_id_current;
    void *udata;
} choker_t;

static unsigned long __peer_hash(const void *obj)
{
    return (unsigned long) obj;
}

static long __peer_compare(const void *obj, const void *other)
{
    return obj - other;
}

void bt_seeding_choker_unchoke_peer(void *ckr, void *peer);

/* @param size : the most number of unchoked peers we can have */
void *bt_seeding_choker_new(const int size)
{
    choker_t *ch;

    ch = calloc(1, sizeof(choker_t));
    ch->max_unchoked_peers = size;
    ch->peers = hashmap_new(__peer_hash, __peer_compare, 11);
    ch->peers_unchoked = llqueue_new();
    ch->peers_choked = llqueue_new();
    return ch;
}

void bt_seeding_choker_add_peer(void *ckr, void *peer)
{
    choker_t *ch = ckr;

    hashmap_put(ch->peers, peer, peer);
    llqueue_offer(ch->peers_choked, peer);
}

void bt_seeding_choker_remove_peer(void *ckr, void *peer)
{
    choker_t *ch = ckr;

    hashmap_remove(ch->peers, peer);
    /* FIXME: shouldn't this peer also be removed from the queues? */
}

static void __choke_peer(choker_t * ch, void *peer)
{
    llqueue_remove_item(ch->peers_unchoked, peer);
    /*  we're back in the queue for being allowed back */
    llqueue_offer(ch->peers_choked, peer);
    ch->iface->choke_peer(ch->udata, peer);
}

void bt_seeding_choker_decide_best_npeers(void *ckr)
{
    choker_t *ch = ckr;

    void *peer;

    /*  if we've already unchoked the number of npeers, no more work to do */
    if (ch->max_unchoked_peers == bt_seeding_choker_get_npeers(ch))
    {
//        return;
    }

    /*  only choke if we are maxed out on unchoked peers */
    if (llqueue_count(ch->peers_unchoked) >= ch->max_unchoked_peers)
    {
        /*  choke most recent */
        peer = llqueue_poll(ch->peers_unchoked);
        __choke_peer(ch, peer);
    }

    /* unchoke peer who has been waiting the longest  */
    peer = llqueue_poll(ch->peers_choked);
    bt_seeding_choker_unchoke_peer(ch, peer);
}

#if 0
void bt_seeding_choker_set_peer(void *cho)
{
// Every 10 seconds...
// It does reciprocation and number of uploads capping by unchoking the four peers which it has the best download rates from and are interested. Peers which have a better upload rate but aren't interested get unchoked and if they become interested the worst uploader gets choked. If a downloader has a complete file, it uses its upload rate rather than its download rate to decide who to unchoke.
// Every 30 seconds...
// optimistically unchoke

}
#endif

void bt_seeding_choker_unchoke_peer(void *ckr, void *peer)
{
    choker_t *ch = ckr;

    peer_t *pr;

    assert(hashmap_contains_key(ch->peers, peer));
    ch->iface->unchoke_peer(ch->udata, peer);

    pr = hashmap_get(ch->peers, peer);

//    /*  find top N-1  */

    /*  ensure that this peer is on the tail of the queue */
    llqueue_remove_item(ch->peers_choked, peer);
    llqueue_remove_item(ch->peers_unchoked, peer);
    llqueue_offer(ch->peers_unchoked, peer);
//    pr->unchoke_id = pr
}

void bt_seeding_choker_set_choker_peer_iface(void *ckr,
                                             void *udata,
                                             bt_choker_peer_i * iface)
{
    choker_t *ch = ckr;

    ch->udata = udata;
    ch->iface = iface;
}

int bt_seeding_choker_get_npeers(void *ckr)
{
    choker_t *ch = ckr;

    return hashmap_count(ch->peers);
}

void bt_seeding_choker_get_iface(bt_choker_i * iface)
{
    iface->new = bt_seeding_choker_new;
    iface->unchoke_peer = bt_seeding_choker_unchoke_peer;
    iface->add_peer = bt_seeding_choker_add_peer;
    iface->remove_peer = bt_seeding_choker_remove_peer;
    iface->get_npeers = bt_seeding_choker_get_npeers;
    iface->set_choker_peer_iface = bt_seeding_choker_set_choker_peer_iface;
    iface->decide_best_npeers = bt_seeding_choker_decide_best_npeers;
}
