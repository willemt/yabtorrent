/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief manage a set of peers
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include "bitfield.h"
#include "pwp_connection.h"

#include "config.h"

#include "bt.h"
#include "bt_string.h"
#include "bt_local.h"
#include "bt_peermanager.h"

#include "linked_list_hashmap.h"

typedef struct {

    void* cfg;
    void* caller;
    void* (*func_peerconn_init)(void* caller);
    hashmap_t *peers;
} bt_peermanager_t;

/**
 * djb2 by Dan Bernstein. */
static unsigned long __peer_hash(const void *obj)
{
    const bt_peer_t* peer = obj;
    const char* str;
    unsigned long hash = 5381;
    int c;
    
    for (str = peer->ip; c = *str++;)
        hash = ((hash << 5) + hash) + c;
    hash += peer->port * 59;
    return hash;
}

static long __peer_compare(const void *obj, const void *other)
{
    const bt_peer_t* p1 = obj;
    const bt_peer_t* p2 = other;
    int i;

    i = strcmp(p1->ip,p2->ip);
    if (i != 0)
        return i;

    return p1->port - p2->port;
}

/**
 * @return 1 if the peer is within the manager */
int bt_peermanager_contains(void *pm, const char *ip, const int port)
{
    bt_peermanager_t *me = pm;
    hashmap_iterator_t iter;

    /* find peerconn that has this ip and port */
    for (hashmap_iterator(me->peers,&iter);
         hashmap_iterator_has_next(me->peers,&iter);)
    {
        bt_peer_t* peer;

        peer = hashmap_iterator_next(me->peers,&iter);
        if (!strcmp(peer->ip, ip) && peer->port == port)
        {
            return 1;
        }
    }
    return 0;
}

/**
 * @return peer that corresponds to conn_ctx, otherwise NULL */
void *bt_peermanager_conn_ctx_to_peer(void * pm, void* conn_ctx)
{
    bt_peermanager_t *me = pm;
    hashmap_iterator_t iter;

    /* find peerconn that has this conn_ctx */
    for (hashmap_iterator(me->peers,&iter);
         hashmap_iterator_has_next(me->peers,&iter);)
    {
        bt_peer_t* peer;

        peer = hashmap_iterator_next(me->peers,&iter);
        if (peer->conn_ctx == conn_ctx)
            return peer;
    }

    return NULL;
}

/**
 * Add the peer.
 * Initiate connection with the peer.
 * @return freshly created bt_peer
 */
bt_peer_t *bt_peermanager_add_peer(void *pm,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port)
{
    bt_peermanager_t *me = pm;
    void *pc;
    bt_peer_t *peer;

    /* prevent dupes.. */
    if (bt_peermanager_contains(me, ip, port))
    {
        return NULL;
    }

    peer = calloc(1, sizeof(bt_peer_t));

    /*  'compact=0'
     *  doesn't use peerids.. */
    if (peer_id)
    {
        asprintf(&peer->peer_id, "00000000000000000000");
    }
    else
    {
        asprintf(&peer->peer_id, "");//, peer_id_len, peer_id);
    }
    asprintf(&peer->ip, "%.*s", ip_len, ip);
    peer->port = port;

#if 0 /*  debug */
    printf("adding peer: ip:%.*s port:%d\n", ip_len, ip, port);
    //__log(bto,NULL,"adding peer: ip:%.*s port:%d\n", ip_len, ip, port);
#endif

    hashmap_put(me->peers,peer,peer);
    return peer;
}

/**
 * Remove the peer.
 * Disconnect the peer
 *
 * @todo add disconnection functionality
 *
 * @return 1 on sucess; otherwise 0
 */
int bt_peermanager_remove_peer(void *pm, bt_peer_t* peer)
{
    bt_peermanager_t *me = pm;

//    bt_leeching_choker_add_peer(me->lchoke, peer);
    hashmap_remove(me->peers,peer);
    return 1;
}

void bt_peermanager_forall(
        void* pm,
        void* caller,
        void* udata,
        void (*run)(void* caller, void* peer, void* udata))
{
    bt_peermanager_t *me = pm;
    hashmap_iterator_t iter;

    for (hashmap_iterator(me->peers,&iter);
         hashmap_iterator_has_next(me->peers,&iter);)
    {
        bt_peer_t* peer;

        peer = hashmap_iterator_next(me->peers,&iter);
        run(caller,peer,udata);
    }
}

int bt_peermanager_count(void* pm)
{
    bt_peermanager_t* me = pm;

    return hashmap_count(me->peers);
}

void bt_peermanager_set_config(void* pm, void* cfg)
{
    bt_peermanager_t* me = pm;

    me->cfg = cfg;
}

void* bt_peermanager_get_peer_from_pc(void* pm, const void* pc)
{
    bt_peermanager_t *me = pm;
    hashmap_iterator_t iter;

    /* find peerconn that has this ip and port */
    for (hashmap_iterator(me->peers,&iter);
         hashmap_iterator_has_next(me->peers,&iter);)
    {
        bt_peer_t* p = hashmap_iterator_next(me->peers,&iter);

        if (p->pc == pc)
        {
            return p;
        }
    }
    return NULL;
}

void* bt_peermanager_new(void* caller)
{
    bt_peermanager_t* me;

    me = calloc(1,sizeof(bt_peermanager_t));
//    me->caller = caller;
//    me->func_peerconn_init = func_peerconn_init;
    me->peers = hashmap_new(__peer_hash, __peer_compare, 11);
    return me;
}
