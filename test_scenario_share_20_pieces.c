
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

/**
 * add a random subset of pieces to the piece db */
static void __add_piece_intersection_of_mocktorrent(void* db, void* db2, void* mt, int npieces)
{
    int ii;

    for (ii=0; ii<npieces; ii++)
    {
        void* data;
        bt_block_t blk;

        /* only add the piece if db2 does not have it */
        if (bt_piece_is_complete(bt_piecedb_get(db2,ii)))
            continue;

        blk.piece_idx = ii;
        blk.offset = 0;
        blk.len = 5;
        data = mocktorrent_get_data(mt,ii);
        bt_diskmem_write_block(bt_piecedb_get_diskstorage(db), NULL, &blk, data);
    }
}

/**
 * add a random subset of pieces to the piece db */
static void __add_random_piece_subset_of_mocktorrent(void* db, void* mt, int npieces)
{
    int ii;

    for (ii=0; ii<npieces; ii++)
    {
        void* data;
        bt_block_t blk;
        int rand_num;
        
        rand_num = rand();

        if (rand_num % 2 == 0)
             continue;

        blk.piece_idx = ii;
        blk.offset = 0;
        blk.len = 5;
        data = mocktorrent_get_data(mt,ii);
        bt_diskmem_write_block(bt_piecedb_get_diskstorage(db), NULL, &blk, data);
    }
}

void TestBT_Peer_share_20_pieces(
    CuTest * tc
)
{
    int num_pieces;
    int ii;
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;
    char *addr;

    num_pieces = 50;
    clients_setup();
    mt = mocktorrent_new(num_pieces,5);
    a = mock_client_setup(5);
    b = mock_client_setup(5);

    for (
        hashmap_iterator(clients_get(), &iter);
        hashmap_iterator_has_next(clients_get(), &iter);
        )
    {
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(clients_get(), &iter);
        bt = cli->bt;

        /* default configuration for clients */
        cfg = bt_dm_get_config(bt);
        config_set_va(cfg, "npieces", "%d", num_pieces);
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");

        /* add files/pieces */
        bt_piecedb_increase_piece_space(bt_dm_get_piecedb(bt),num_pieces * 5);
        for (ii=0; ii<num_pieces; ii++)
        {
            char hash[21];
            void* pd;
            
            pd = bt_dm_get_piecedb(bt);
            mocktorrent_get_piece_sha1(mt,hash,ii);
            bt_piecedb_add(pd,hash);
        }
    }
  
    /* B will initiate the connection */
    asprintf(&addr,"%p", b);
    client_add_peer(a,NULL,0,addr,strlen(addr),0);

    __add_random_piece_subset_of_mocktorrent(
            bt_dm_get_piecedb(a->bt),
            mt, num_pieces);

    __add_piece_intersection_of_mocktorrent(
            bt_dm_get_piecedb(b->bt),
            bt_dm_get_piecedb(a->bt),
            mt, num_pieces);

//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(a->bt));
//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(b->bt));

    for (ii=0; ii<80; ii++)
    {
#if 0 /* debugging */
        printf("\nStep %d:\n", ii+1);
#endif

        bt_dm_periodic(a->bt, NULL);
        bt_dm_periodic(b->bt, NULL);

        network_poll(a->bt, &a, 0,
                bt_dm_dispatch_from_buffer,
                bt_dm_peer_connect);

        network_poll(b->bt, &b, 0,
                bt_dm_dispatch_from_buffer,
                bt_dm_peer_connect);
//        __print_client_contents();
//        bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(b->bt));
    }

//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(a->bt));
//    bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(b->bt));

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_dm_get_piecedb(a->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_dm_get_piecedb(b->bt)));
}
