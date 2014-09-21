
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
#include "network_adapter.h"
#include "network_adapter_mock.h"
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
 * Does the bt_dm_check_pieces() confirm if a piece is complete? */
void TestBT_Peer_shares_all_pieces_between_each_other(
    CuTest * tc
    )
{
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;

    clients_setup();
    mt = mocktorrent_new(2, 5);
    a = mock_client_setup(5);
    b = mock_client_setup(5);

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
        config_set(cfg, "npieces", "2");
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");
        /* add files/pieces */
        bt_piecedb_increase_piece_space(bt_dm_get_piecedb(bt), 5);
        bt_piecedb_increase_piece_space(bt_dm_get_piecedb(bt), 12);

        bt_piecedb_add_with_hash_and_size(bt_dm_get_piecedb(bt),
                                          mocktorrent_get_piece_sha1(mt, hash,
                                                                     0), 5);
        bt_piecedb_add_with_hash_and_size(bt_dm_get_piecedb(bt),
                                          mocktorrent_get_piece_sha1(mt, hash,
                                                                     1), 5);
    }

    /* write blocks to client A & B */
    {
        void* data;
        bt_block_t blk;

        blk.piece_idx = 0;
        blk.offset = 0;
        blk.len = 5;

        data = mocktorrent_get_data(mt, 0);
        bt_diskmem_write_block(
            bt_piecedb_get_diskstorage(bt_dm_get_piecedb(b->bt)),
            NULL, &blk, data);

        blk.piece_idx = 1;
        data = mocktorrent_get_data(mt, 1);
        bt_diskmem_write_block(
            bt_piecedb_get_diskstorage(bt_dm_get_piecedb(a->bt)),
            NULL, &blk, data);
    }

    bt_dm_check_pieces(a->bt);
    bt_dm_check_pieces(b->bt);

    bt_dm_periodic(a->bt, NULL);
    bt_dm_periodic(b->bt, NULL);

    CuAssertTrue(tc, bt_dm_piece_is_complete(a->bt, 1));
    CuAssertTrue(tc, bt_dm_piece_is_complete(b->bt, 0));
}

