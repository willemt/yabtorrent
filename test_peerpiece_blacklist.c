
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

void TestPeerpieceblacklist_init_has_no_peers(
    CuTest * tc
)
{
    bt_pp_blacklist *b;

    b = bt_pp_blacklist_new();
    CuAssertTrue(tc, 0 == bt_pp_blacklist_num_peers(b));
}

void TestPeerpieceblacklist_add_peerpiece(
    CuTest * tc
)
{
    bt_pp_blacklist *b;
    char* peer = "a";

    b = bt_pp_blacklist_new();
    bt_pp_blacklist_add(b,peer,1);
    CuAssertTrue(tc, 0 == bt_pp_blacklist_num_peers(b));
}

void TestBT_GeneratedPeeridIs20BytesLong(
    CuTest * tc
)
{
    char *peerid;

    peerid = bt_generate_peer_id();

    CuAssertTrue(tc, 20 == strlen(peerid));
