
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

#include "block.h"
#include "bt.h"
#include "networkfuncs.h"
#include "networkfuncs_mock.h"
#include "mock_torrent.h"
#include "mock_client.h"

#include "bt_piece_db.h"
#include "bt_diskmem.h"
#include "config.h"
#include "linked_list_hashmap.h"
#include "bipbuffer.h"

#include <fcntl.h>
#include <sys/time.h>

/** a have message has to be sent by client B, for client A and C to finish */
void TestBT_Peer_three_share_all_pieces_between_each_other(
    CuTest * tc
)
{
    int ii;
    client_t* a, *b, *c;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;
    char *addr;

    printf("d\n");
    clients_setup();
    printf("d\n");
    mt = mocktorrent_new(3,5);
    printf("d\n");
    a = mock_client_setup(5);
    printf("d\n");
    b = mock_client_setup(5);
    printf("d\n");
    c = mock_client_setup(5);
    printf("d\n");

    for (
        hashmap_iterator(clients_get(), &iter);
        hashmap_iterator_has_next(clients_get(), &iter);
        )
    {
        char hash[21];
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(clients_get(), &iter);
        bt = cli->bt;
        cfg = bt_dm_get_config(bt);
        /* default configuration for clients */
        config_set(cfg, "npieces", "3");
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");
        /* add files/pieces */
        //bt_piecedb_add_file(bt_dm_get_piecedb(bt),"test.txt",8,20);
        bt_piecedb_increase_piece_space(bt_dm_get_piecedb(bt),20);
//        bt_piecedb_add_file(bt_dm_get_piecedb(bt),"test2.txt",8,12);
        bt_piecedb_add(bt_dm_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,hash,0));
        bt_piecedb_add(bt_dm_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,hash,1));
        bt_piecedb_add(bt_dm_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,hash,2));
    }

    /* write blocks to client A & B */
    {
        void* data;
        bt_block_t blk;

        blk.piece_idx = 0;
        blk.offset = 0;
        blk.len = 5;

        data = mocktorrent_get_data(mt,0);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_dm_get_piecedb(a->bt)),
                NULL, &blk, data);

        blk.piece_idx = 1;
        data = mocktorrent_get_data(mt,1);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_dm_get_piecedb(b->bt)),
                NULL, &blk, data);

        blk.piece_idx = 2;
        data = mocktorrent_get_data(mt,2);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_dm_get_piecedb(c->bt)),
                NULL, &blk, data);
    }

    /* B will initiate the connection */
    asprintf(&addr,"%p", b);
    client_add_peer(a,NULL,0,addr,strlen(addr),0);
    asprintf(&addr,"%p", c);
    client_add_peer(b,NULL,0,addr,strlen(addr),0);

//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(a->bt));
//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(b->bt));
//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(c->bt));

    for (ii=0; ii<10; ii++)
    {
#if 0 /* debugging */
        printf("\nStep %d:\n", ii+1);
#endif

        bt_dm_periodic(a->bt, NULL);
        bt_dm_periodic(b->bt, NULL);
        bt_dm_periodic(c->bt, NULL);

        network_poll(a->bt, &a, 0,
                bt_dm_dispatch_from_buffer,
                bt_dm_peer_connect);

        network_poll(b->bt, &b, 0,
                bt_dm_dispatch_from_buffer,
                bt_dm_peer_connect);

        network_poll(c->bt, &c, 0,
                bt_dm_dispatch_from_buffer,
                bt_dm_peer_connect);
//        __print_client_contents();
    }

//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(a->bt));
//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(b->bt));
//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(c->bt));

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_dm_get_piecedb(a->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_dm_get_piecedb(b->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_dm_get_piecedb(c->bt)));
}

