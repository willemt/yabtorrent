
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "block.h"
#include "bt.h"
#include "bt_local.h"

/*
 * bt_sha1_equal tells if a sha1 hash is equal or not
 */
void TestBT_Sha1Equal(
    CuTest * tc
)
{
    char *s1 = "00000000000000000000";

    char *s2 = "00000000000000000000";

    char *s3 = "10000000000000000000";

    CuAssertTrue(tc, 1 == bt_sha1_equal(s1, s2));
    CuAssertTrue(tc, 0 == bt_sha1_equal(s1, s3));
}

/*
 * bt_client_add_peer adds a peer to the peer database
 */
void TestBT_client_add_peer(
    CuTest * tc
)
{
    void *id;
    char *peerid = "0000000000000";
    char *ip = "127.0.0.1";

    id = bt_client_new();
    /*  a peer id is required for adding peers */
    //bt_client_set_peer_id(id, peerid);

    CuAssertTrue(tc, 0 == bt_client_get_num_peers(id));

    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);

    CuAssertTrue(tc, 1 == bt_client_get_num_peers(id));
}

#if 0 /* tested as part of peer manager */
void T_estBT_ClientCantAddPeerTwice(
    CuTest * tc
)
{
    void *id;

    char *peerid = "0000000000000";

    char *ip = "127.0.0.1";

    id = bt_client_new();
    /*  a peer id is required for adding peers */
    //bt_client_set_peer_id(id, peerid);

    CuAssertTrue(tc, 0 == bt_client_get_num_peers(id));

    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    CuAssertTrue(tc, 1 == bt_client_get_num_peers(id));
}
#endif

void TestBT_GeneratedPeeridIs20BytesLong(
    CuTest * tc
)
{
    char *peerid;

    peerid = bt_generate_peer_id();

    CuAssertTrue(tc, 20 == strlen(peerid));
}
