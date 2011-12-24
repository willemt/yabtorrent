/*
 * =====================================================================================
 *
 *       Filename:  test_choker.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/29/11 23:13:16
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
#include "CuTest.h"

#include <stdint.h>

#include "bt.h"
#include "bt_local.h"


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
    int isInterested,
    int isChoked
)
{
    pr->drate = drate;
    pr->urate = urate;
    pr->isInterested = isInterested;
    pr->isChoked = isChoked;
}

/*----------------------------------------------------------------------------*/

int __get_drate(
    const void *data,
    const void *p
)
{
    const peer_t *pr = p;

    return pr->drate;
}

int __get_urate(
    const void *data,
    const void *p
)
{
    const peer_t *pr = p;

    return pr->urate;
}

int __get_is_interested(
    void *data,
    void *p
)
{
    peer_t *pr = p;

    return pr->isInterested;
}

void __choke_peer(
    void *data,
    void *p
)
{
    peer_t *pr = p;

    pr->isChoked = 1;
}

void __unchoke_peer(
    void *data,
    void *p
)
{
    peer_t *pr = p;

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
void TestBTSeedingChoker_add_peer(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    cr = bt_seeding_choker_new(3);
    CuAssertTrue(tc, 0 == bt_seeding_choker_get_npeers(cr));
    bt_seeding_choker_add_peer(cr, &peers[0]);
    CuAssertTrue(tc, 1 == bt_seeding_choker_get_npeers(cr));
}

void TestBTSeedingChoker_remove_peer(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    cr = bt_seeding_choker_new(3);
    bt_seeding_choker_add_peer(cr, &peers[0]);
    bt_seeding_choker_remove_peer(cr, &peers[0]);
    CuAssertTrue(tc, 0 == bt_seeding_choker_get_npeers(cr));
}

/*----------------------------------------------------------------------------*/
void TestBTSeedingChoker_decide_unchokes_interested_choked(
    CuTest * tc
)
{
    void *cr;

    peer_t peers[10];

    __pset(&peers[0], 0, 100, 1, 1);
    __pset(&peers[1], 0, 50, 1, 1);
    __pset(&peers[2], 0, 200, 1, 1);
    __pset(&peers[3], 0, 10, 1, 1);

    cr = bt_seeding_choker_new(3);
    bt_seeding_choker_set_choker_peer_iface(cr, NULL, &iface_choker_peer);
    bt_seeding_choker_add_peer(cr, &peers[0]);
    bt_seeding_choker_add_peer(cr, &peers[1]);
    bt_seeding_choker_add_peer(cr, &peers[2]);
    bt_seeding_choker_add_peer(cr, &peers[3]);
    bt_seeding_choker_unchoke_peer(cr, &peers[0]);
    bt_seeding_choker_unchoke_peer(cr, &peers[1]);
    bt_seeding_choker_unchoke_peer(cr, &peers[2]);
    /* the unchoked and interested remote peers are ordered according to the
     * time they were last unchoked, most recently unchoked peers first.
     * The 3 first peers are kept unchoked and an additional 4th peer that is
     * choked and interested is selected at random and unchoked.*/
    bt_seeding_choker_decide_best_npeers(cr);

//    printf("%d,%d,%d,%d\n", peers[0].isChoked, peers[1].isChoked,
//           peers[2].isChoked, peers[3].isChoked);

    CuAssertTrue(tc, 1 == peers[0].isChoked);
    CuAssertTrue(tc, 0 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 0 == peers[3].isChoked);

    bt_seeding_choker_decide_best_npeers(cr);
    CuAssertTrue(tc, 0 == peers[0].isChoked);
    CuAssertTrue(tc, 1 == peers[1].isChoked);
    CuAssertTrue(tc, 0 == peers[2].isChoked);
    CuAssertTrue(tc, 0 == peers[3].isChoked);
}
