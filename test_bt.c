
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
 * response to a tracker response header will result in disconnect if the compulsory keys are not included
 */
void TestBTClient_disconnect_if_compulsory_keys_not_included_in_tracker_response(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0);
}

/*
 * peerid can't be set to higher than 20 bytes
 */
void TestBTClient_peerid_cant_be_set_higher_than_20_bytes(
    CuTest * tc
)
{
    void *id;

    char *s1 = "000000000000000000001";

    id = bt_client_new();
    bt_client_set_peer_id(id, s1);
    CuAssertTrue(tc, 0 != strcmp(s1, (char *) bt_client_get_peer_id(id)));
}

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
    bt_client_set_peer_id(id, peerid);

    CuAssertTrue(tc, 0 == bt_client_get_num_peers(id));

    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);

    CuAssertTrue(tc, 1 == bt_client_get_num_peers(id));
}

void TestBT_ClientCantAddPeerTwice(
    CuTest * tc
)
{
    void *id;

    char *peerid = "0000000000000";

    char *ip = "127.0.0.1";

    id = bt_client_new();
    /*  a peer id is required for adding peers */
    bt_client_set_peer_id(id, peerid);

    CuAssertTrue(tc, 0 == bt_client_get_num_peers(id));

    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    CuAssertTrue(tc, 1 == bt_client_get_num_peers(id));
}

void TestBT_ClientCantAddSelfAsPeer(
    CuTest * tc
)
{
    void *id;

    char *peerid = "0000000000000";

    char *ip = "127.0.0.1";

    id = bt_client_new();
    /*  a peer id is required for adding peers */
    bt_client_set_peer_id(id, peerid);
    //bt_client_set_opt(id, "pwp_listen_port", "4000");
    /*  add self */
    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    CuAssertTrue(tc, 0 == bt_client_get_num_peers(id));
}

void TestBT_ClientRemovePeer(
    CuTest * tc
)
{
    void *id;

    char *peerid = "0000000000000";

    char *ip = "127.0.0.1";

    id = bt_client_new();
    /*  a peer id is required for adding peers */
    bt_client_set_peer_id(id, peerid);

    CuAssertTrue(tc, 0 == bt_client_get_num_peers(id));

    bt_client_add_peer(id, peerid, strlen(peerid), ip, strlen(ip), 4000);
    bt_client_remove_peer(id, peerid);
    CuAssertTrue(tc, 0 == bt_client_get_num_peers(id));
}

void TestBT_SetFailed(
    CuTest * tc
)
{
    void *id;

    id = bt_client_new();

    bt_client_set_failed(id, "just not good");

    CuAssertTrue(tc, bt_client_is_failed(id));
    CuAssertTrue(tc,
                 0 == strcmp(bt_client_get_fail_reason(id), "just not good"));
}

void TestBT_SetInfoHash(
    CuTest * tc
)
{
    void *id;

    char *info_hash = "8%ea%b8%dc%d0%f0%062jb%b5%06%09%12%c8%f0dZ%84%c4";

    id = bt_client_new();

    //bt_client_set_opt(id, "infohash", info_hash, 20);

    CuAssertTrue(tc,0);
//    CuAssertTrue(tc,
//                 0 == strcmp(bt_client_get_opt_string(id, "infohash"), info_hash));
}

void TestBT_GeneratedPeeridIs20BytesLong(
    CuTest * tc
)
{
    char *peerid;

    peerid = bt_generate_peer_id();

    CuAssertTrue(tc, 20 == strlen(peerid));
}

/*----------------------------------------------------------------------------*/
void TestBT_AddPiece(
    CuTest * tc
)
{
    void *id;

    char *piece = "0000000000000000000000000000";

    id = bt_client_new();

    bt_client_add_pieces(id, piece, 1);
    CuAssertTrue(tc, 1 == bt_client_get_num_pieces(id));
}

#if 0
void TxestBT_AddPieceLastPieceIsProperlySized(
    CuTest * tc
)
{
    void *id;

    char *piece = "0000000000000000000000000000";

    id = bt_init();
    bt_set_piece_length(id, 75);
    bt_add_file(id, "test.xml", strlen("test.xml"), 100);
    bt_add_piece(id, piece);
    CuAssertTrue(tc, 1 == bt_get_num_peers(id));
}
#endif

/*----------------------------------------------------------------------------*/

void TestBT_AddingFileIncreasesTotalFileSize(
    CuTest * tc
)
{
    void *id = bt_client_new();

    CuAssertTrue(tc, 0 == bt_client_get_total_file_size(id));

    bt_client_add_file(id, "test.xml", strlen("test.xml"), 100);
    CuAssertTrue(tc, 100 == bt_client_get_total_file_size(id));

    bt_client_add_file(id, "test2.xml", strlen("test2.xml"), 100);
    CuAssertTrue(tc, 200 == bt_client_get_total_file_size(id));
}

#if 0
void TxestBTClient_downtracker(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0 == strcmp(port, "9000"));
}
#endif
