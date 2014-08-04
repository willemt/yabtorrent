
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <CuTest.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include "bt.h"
#include "bt_selector_random.h"
#include "network_adapter.h"
#include "network_adapter_mock.h"
#include "mock_torrent.h"
#include "mock_client.h"

/* for handshaker funcs */
#include "pwp_handshaker.h"

#include "bt_piece_db.h"
#include "bt_diskmem.h"
#include "config.h"
#include "linked_list_hashmap.h"
#include "bipbuffer.h"

#include <fcntl.h>
#include <sys/time.h>

void *__clients = NULL;

static void* call_exclusively_pass_through(void* me, void* cb_ctx, void **lock, void* udata,
        void* (*cb)(void* me, void* udata))
{
    return cb(me,udata);
}

static unsigned long __vptr_hash(
    const void *e1
)
{
    return (unsigned long)e1;
}

static long __vptr_compare(
    const void *e1,
    const void *e2
)
{
    return (unsigned long)e1 - (unsigned long)e2;
}

static void __log(void *udata, void *src, const char *buf, ...)
{
//    printf("%s\n", buf);
}

void* clients_get()
{
    return __clients;
}

void* clients_setup()
{
    __clients = hashmap_new(__vptr_hash, __vptr_compare, 11);

    return NULL;
}

/** create disk backend */
void mock_client_setup_disk_backend(void* bt, unsigned int piece_len)
{
    void* dc, *db;

    dc = bt_diskmem_new();
    bt_diskmem_set_size(dc, piece_len);
    db = bt_piecedb_new();
    bt_piecedb_set_diskstorage(db, bt_diskmem_get_blockrw(dc), dc);
    //bt_piecedb_set_piece_length(db,piece_len);
    bt_dm_set_piece_db(bt, &((bt_piecedb_i){ .get_piece = bt_piecedb_get}),db);
}

void client_add_peer(
    client_t* me,
    char* peer_id,
    unsigned int peer_id_len,
    char* ip,
    unsigned int ip_len,
    unsigned int port)
{
    void* peer;
    void* peer_nethandle;
    char ip_string[32];

    sprintf(ip_string,"%.*s", ip_len, ip);
    peer_nethandle = NULL;
    
#if 0

    /* connect to the peer */
    if (0 == peer_connect(me,
                (void**)&me,
                &peer_nethandle,
                ip_string, port,
                bt_dm_dispatch_from_buffer,
                bt_dm_peer_connect,
                bt_dm_peer_connect_fail))
    {
        printf("failed connection to peer");
    }
#endif

    peer = bt_dm_add_peer(me->bt, peer_id, peer_id_len, ip, ip_len, port,
            peer_nethandle, NULL);

}

client_t* mock_client_setup(int piecelen)
{
    client_t* cli;
    config_t* cfg;

    cli = networkfuns_mock_client_new(NULL);

    /* bittorrent client */
    cli->bt = bt_dm_new();
    cfg = bt_dm_get_config(cli->bt);
    config_set(cfg, "my_peerid", bt_generate_peer_id());
    bt_dm_set_cbs(cli->bt,
        &((bt_dm_cbs_t) {
        .peer_connect = peer_connect,
        .peer_send = peer_send,
        .peer_disconnect = peer_disconnect,
        .call_exclusively = call_exclusively_pass_through,
        .log = __log,
        .handshaker_new = pwp_handshaker_new,
        .handshaker_release = pwp_handshaker_release,
        .handshaker_dispatch_from_buffer = pwp_handshaker_dispatch_from_buffer,
        .send_handshake = pwp_send_handshake,
        .msghandler_new = NULL
        }), cli);
    bt_dm_set_piece_selector(cli->bt, 
        &((bt_pieceselector_i) {
        .new = bt_random_selector_new,
        .peer_giveback_piece = bt_random_selector_giveback_piece,
        .have_piece = bt_random_selector_have_piece,
        .remove_peer = bt_random_selector_remove_peer,
        .add_peer = bt_random_selector_add_peer,
        .peer_have_piece = bt_random_selector_peer_have_piece,
        .get_npeers = bt_random_selector_get_npeers,
        .get_npieces = bt_random_selector_get_npieces,
        .poll_piece = bt_random_selector_poll_best_piece
        }), NULL);
    mock_client_setup_disk_backend(cli->bt, piecelen);

    hashmap_put(__clients, cli, cli);

    return cli;
}

void mock_on_connect(void *bt, void* nethandle, char *ip, int port)
{
    bt_dm_add_peer(bt, "", 0, ip, strlen(ip), port, nethandle, NULL);
    bt_dm_peer_connect(bt, nethandle, ip, port);
}

