/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "bt.h"
#include "bt_choker_peer.h"
#include "bt_choker.h"
#include "bt_choker_leecher.h"

#if 0
void TxestBTPiece_new_has_set_size_and_hashsum(
    CuTest * tc
)
{
    bt_piece_t *pce;

    pce = bt_piece_new(HASH_EXAMPLE, 40);
    CuAssertTrue(tc, 0 == strncmp(bt_piece_get_hash(pce), HASH_EXAMPLE, 20));
    CuAssertTrue(tc, 40 == bt_piece_get_size(pce));
}
#endif

typedef struct
{

    int drate;
    int urate;
    int isInterested;
    int isChoked;
} peer_t;

static void __pset(
    peer_t * pr,
    int drate,
    int urate,
    int isInterested
)
{
    pr->drate = drate;
    pr->urate = urate;
    pr->isInterested = isInterested;
}

int __get_drate(
    const void *udata,
    const void *p
)
{
    const peer_t *pr = p;

    assert(pr);
    return pr->drate;
}

int __get_urate(
    const void *udata,
    const void *p
)
{
    const peer_t *pr = p;

    assert(pr);
    return pr->urate;
}

int __get_is_interested(
    void *udata,
    void *pro
)
{
    peer_t *pr = pro;

    return pr->isInterested;
}

void __choke_peer(
    void *udata,
    void *pro
)
{
    peer_t *pr = pro;

    pr->isChoked = 1;
}

void __unchoke_peer(
    void *udata,
    void *pro
)
{
    peer_t *pr = pro;

    pr->isChoked = 0;
}

bt_choker_peer_i iface_choker_peer = {
    .get_drate = __get_drate,
    .get_urate = __get_urate,
    .get_is_interested = __get_is_interested,
    .choke_peer = __choke_peer,
    .unchoke_peer = __unchoke_peer
};


/*----------------------------------------------------------------------------*/

void TestBTleechingChoke_add_peer(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    cr = bt_leeching_choker_new(3);
    CuAssertTrue(tc, 0 == bt_leeching_choker_get_npeers(cr));
    bt_leeching_choker_add_peer(cr, &peers[0]);
    CuAssertTrue(tc, 1 == bt_leeching_choker_get_npeers(cr));
}

void TestBTleechingChoke_cant_add_peer_more_than_once(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    CuAssertTrue(tc, 1 == bt_leeching_choker_get_npeers(cr));
}

void TestBTleechingChoke_remove_peer(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_remove_peer(cr, &peers[0]);
    CuAssertTrue(tc, 0 == bt_leeching_choker_get_npeers(cr));
}

void TestBTleechingChoke_select_best_4_peers_based_off_upload(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    __pset(&peers[0], 0, 100, 1);
    __pset(&peers[1], 0, 50, 1);
    __pset(&peers[2], 0, 65, 1);
    __pset(&peers[3], 0, 0, 1);


    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_set_choker_peer_iface(cr, NULL, &iface_choker_peer);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_add_peer(cr, &peers[1]);
    bt_leeching_choker_add_peer(cr, &peers[2]);
    bt_leeching_choker_add_peer(cr, &peers[3]);
    bt_leeching_choker_decide_best_npeers(cr);

//    CuAssertTrue(tc, &peers[0] == bt_leeching_choker_get_best_peer(pce));

    CuAssertTrue(tc, 0 == peers[0].isChoked);
    CuAssertTrue(tc, 0 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 1 == peers[3].isChoked);
}

void TestBTleechingChoke_select_drop_bad_uploader(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    __pset(&peers[0], 0, 100, 1);
    __pset(&peers[1], 0, 50, 1);
    __pset(&peers[2], 0, 200, 1);
    __pset(&peers[3], 0, 25, 1);

    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_set_choker_peer_iface(cr, NULL, &iface_choker_peer);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_add_peer(cr, &peers[1]);
    bt_leeching_choker_add_peer(cr, &peers[2]);
    bt_leeching_choker_add_peer(cr, &peers[3]);
    bt_leeching_choker_decide_best_npeers(cr);

    /*  this peer is not good enough any more */
    __pset(&peers[0], 0, 10, 1);
    bt_leeching_choker_decide_best_npeers(cr);
    CuAssertTrue(tc, 1 == peers[0].isChoked);
    CuAssertTrue(tc, 0 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 0 == peers[3].isChoked);
}

void TestBTleechingChoke_drop_bad_uploader_when_interested_is_announced(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    __pset(&peers[0], 0, 100, 1);
    __pset(&peers[1], 0, 50, 1);
    __pset(&peers[2], 0, 200, 1);
    __pset(&peers[3], 0, 75, 0);

    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_set_choker_peer_iface(cr, NULL, &iface_choker_peer);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_add_peer(cr, &peers[1]);
    bt_leeching_choker_add_peer(cr, &peers[2]);
    bt_leeching_choker_add_peer(cr, &peers[3]);
    bt_leeching_choker_decide_best_npeers(cr);
    CuAssertTrue(tc, 0 == peers[3].isChoked);

    /* recently interested peer will take over bad uploader */
    peers[3].isInterested = 1;
    bt_leeching_choker_announce_interested_peer(cr, &peers[3]);
    bt_leeching_choker_decide_best_npeers(cr);
    CuAssertTrue(tc, 0 == peers[0].isChoked);
    CuAssertTrue(tc, 1 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 0 == peers[3].isChoked);
}

void TestBTleechingChoke_optimistically_unchoke(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    __pset(&peers[0], 0, 100, 1);
    __pset(&peers[1], 0, 50, 1);
    __pset(&peers[2], 0, 200, 1);
    __pset(&peers[3], 0, 10, 1);

    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_set_choker_peer_iface(cr, NULL, &iface_choker_peer);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_add_peer(cr, &peers[1]);
    bt_leeching_choker_add_peer(cr, &peers[2]);
    bt_leeching_choker_add_peer(cr, &peers[3]);
    bt_leeching_choker_decide_best_npeers(cr);
    CuAssertTrue(tc, 1 == peers[3].isChoked);

    bt_leeching_choker_optimistically_unchoke(cr);
    CuAssertTrue(tc, 0 == peers[0].isChoked);
    CuAssertTrue(tc, 1 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 0 == peers[3].isChoked);
}

void TestBTleechingChoke_dont_optimistically_unchoke_uninterested(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    __pset(&peers[0], 0, 100, 1);
    __pset(&peers[1], 0, 50, 1);
    __pset(&peers[2], 0, 200, 1);
    __pset(&peers[3], 0, 10, 0);

    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_set_choker_peer_iface(cr, NULL, &iface_choker_peer);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_add_peer(cr, &peers[1]);
    bt_leeching_choker_add_peer(cr, &peers[2]);
    bt_leeching_choker_add_peer(cr, &peers[3]);
    bt_leeching_choker_decide_best_npeers(cr);
    CuAssertTrue(tc, 1 == peers[3].isChoked);

    bt_leeching_choker_optimistically_unchoke(cr);
    CuAssertTrue(tc, 0 == peers[0].isChoked);
    CuAssertTrue(tc, 0 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 1 == peers[3].isChoked);
}

void TestBTleechingChoke_optimistically_unchoke_different(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    __pset(&peers[0], 0, 100, 1);
    __pset(&peers[1], 0, 50, 1);
    __pset(&peers[2], 0, 200, 1);
    __pset(&peers[3], 0, 10, 1);

    cr = bt_leeching_choker_new(3);
    bt_leeching_choker_set_choker_peer_iface(cr, NULL, &iface_choker_peer);
    bt_leeching_choker_add_peer(cr, &peers[0]);
    bt_leeching_choker_add_peer(cr, &peers[1]);
    bt_leeching_choker_add_peer(cr, &peers[2]);
    bt_leeching_choker_add_peer(cr, &peers[3]);
    bt_leeching_choker_decide_best_npeers(cr);
    CuAssertTrue(tc, 1 == peers[3].isChoked);

    bt_leeching_choker_optimistically_unchoke(cr);
    bt_leeching_choker_optimistically_unchoke(cr);
    CuAssertTrue(tc, 0 == peers[0].isChoked);
    CuAssertTrue(tc, 0 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 1 == peers[3].isChoked);
}
