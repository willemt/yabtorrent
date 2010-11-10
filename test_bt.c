
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

void TestBTClientAddPeer(
    CuTest * tc
)
{
    int id;

    char *peerid = "0000000000000";

    char *ip = "127.0.0.1";

    id = bt_init();

    CuAssertTrue(tc, 0 == bt_get_num_peers(id));

    bt_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);

    CuAssertTrue(tc, 1 == bt_get_num_peers(id));
}

void TestBTClientCantAddPeerTwice(
    CuTest * tc
)
{
    int id;

    char *peerid = "0000000000000";

    char *ip = "127.0.0.1";

    id = bt_init();

    CuAssertTrue(tc, 0 == bt_get_num_peers(id));

    int res;

    bt_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    res = bt_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);

    CuAssertTrue(tc, 1 == bt_get_num_peers(id));
    CuAssertTrue(tc, -1 == res);
}

void TestBTClientRemovePeer(
    CuTest * tc
)
{
    int id;

    char *peerid = "0000000000000";

    char *ip = "127.0.0.1";

    id = bt_init();

    CuAssertTrue(tc, 0 == bt_get_num_peers(id));

    bt_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    bt_remove_peer(id, peerid);

    CuAssertTrue(tc, 0 == bt_get_num_peers(id));
}

void TestBTSetFailed(
    CuTest * tc
)
{
    int id;

    id = bt_init();

    bt_set_failed(id, "just not good");

    CuAssertTrue(tc, bt_is_failed(id));
    CuAssertTrue(tc, !strcmp(bt_get_fail_reason(id), "just not good"));
}

void TestBTSetInterval(
    CuTest * tc
)
{
    int id;

    id = bt_init();

    bt_set_interval(id, 1000);

    CuAssertTrue(tc, bt_get_interval(id) == 1000);
}

void TestBTSetInfoHash(
    CuTest * tc
)
{
    int id;

    char *info_hash = "8%ea%b8%dc%d0%f0%062jb%b5%06%09%12%c8%f0dZ%84%c4";

    id = bt_init();

    bt_set_info_hash(id, info_hash);

    CuAssertTrue(tc, !strcmp(bt_get_info_hash(id), info_hash));
}

void TestBTGeneratedPeeridIs20BytesLong(
    CuTest * tc
)
{
    char *peerid;

    peerid = bt_generate_peer_id();

    CuAssertTrue(tc, 20 == strlen(peerid));
}

void TestURL2Host(
    CuTest * tc
)
{
    char *url;

    url = url2host("http://www.abc.com/");

    CuAssertTrue(tc, !strcmp(url, "www.abc.com"));
}

void TestURL2Host2(
    CuTest * tc
)
{
    char *url;

    url = url2host("http://www.abc.com");

    CuAssertTrue(tc, !strcmp(url, "www.abc.com"));
}

void TestBTAddPiece(
    CuTest * tc
)
{
    int id;

    char *piece = "0000000000000000000000000000";

    id = bt_init();

    bt_add_piece(id, piece);
    CuAssertTrue(tc, 1 == bt_get_num_peers(id));
}
