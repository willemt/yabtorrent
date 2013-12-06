
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

