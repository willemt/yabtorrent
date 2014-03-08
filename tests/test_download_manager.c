
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "bt.h"

#if 0
/*
 * bt_dm_add_peer adds a peer to the peer database
 */
void TxestBT_dm_add_peer(
    CuTest * tc
)
{
    void *id;
    char *peerid = "0000000000000";
    char *ip = "127.0.0.1";

    id = bt_dm_new();

    CuAssertTrue(tc, 0 == bt_dm_get_num_peers(id));

    bt_dm_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000, NULL);

    CuAssertTrue(tc, 1 == bt_dm_get_num_peers(id));
}
#endif

void TestBT_dm_dont_add_myself_as_a_peer(
    CuTest * tc
)
{
    void *id;
    char *peerid = "0000000000000";
    char *ip = "192.168.1.1";
    void* peer_ctx = malloc(1);

    id = bt_dm_new();
    CuAssertTrue(tc, NULL != bt_dm_add_peer(id, peerid, strlen(peerid),
                ip, strlen(ip), 4001, peer_ctx, NULL));
}

void TestBT_dm_add_peer_adds_peer(
    CuTest * tc
)
{
    void *id;
    char *peerid = "0000000000000";
    char *ip = "192.168.1.1";
    void* peer_ctx = malloc(1);

    id = bt_dm_new();
    CuAssertTrue(tc, 0 == bt_dm_get_num_peers(id));
    CuAssertTrue(tc, NULL != bt_dm_add_peer(id, peerid, strlen(peerid),
                ip, strlen(ip), 4001, peer_ctx, NULL));
    CuAssertTrue(tc, 1 == bt_dm_get_num_peers(id));
}

void TestBT_dm_dont_add_peer_twice(
    CuTest * tc
)
{
    void *id;
    char *peerid = "0000000000000";
    char *ip = "192.168.1.1";
    void* peer_ctx = malloc(1);

    id = bt_dm_new();
    CuAssertTrue(tc, 0 == bt_dm_get_num_peers(id));
    CuAssertTrue(tc, NULL != bt_dm_add_peer(id, peerid, strlen(peerid),
                ip, strlen(ip), 4001, peer_ctx, NULL));
    CuAssertTrue(tc, NULL == bt_dm_add_peer(id, peerid, strlen(peerid),
                ip, strlen(ip), 4001, peer_ctx, NULL));
    CuAssertTrue(tc, 1 == bt_dm_get_num_peers(id));
}

void TestBT_dm_wont_connect_peer_if_not_added(
    CuTest * tc
)
{
    void *id;
    char *peerid = "0000000000000";
    char *ip = "192.168.1.1";
    void* peer_ctx = malloc(1);

    id = bt_dm_new();
    CuAssertTrue(tc, 0 == bt_dm_peer_connect(id, peer_ctx, ip, 4001));
    CuAssertTrue(tc, NULL != bt_dm_add_peer(id, peerid, strlen(peerid),
                ip, strlen(ip), 4001, peer_ctx, NULL));
    CuAssertTrue(tc, 1 == bt_dm_peer_connect(id, peer_ctx, ip, 4001));
}
