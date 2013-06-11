
/**
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include "mock_torrent.h"

/*----------------------------------------------------------------------------*/

#include "bt_piece_db.h"
#include "bt_diskmem.h"
#include "config.h"
#include "linked_list_hashmap.h"
#include "bipbuffer.h"
#include "networkfuncs_mock.h"

#include <fcntl.h>
#include <sys/time.h>

static void __log(void *udata, void *src, char *buf)
{
    char stamp[32];
    int fd = (unsigned long) udata;
    struct timeval tv;

#if 0 /* debugging */
    printf(buf);
    printf("\n");
#endif

    gettimeofday(&tv, NULL);
    sprintf(stamp, "%d,%0.2f,", (int) tv.tv_sec, (float) tv.tv_usec / 100000);
    write(fd, stamp, strlen(stamp));
    write(fd, buf, strlen(buf));
    write(fd, "\n", 1);
}

bt_piecedb_i pdb_funcs = {
    .poll_best_from_bitfield = bt_piecedb_poll_best_from_bitfield,
    .get_piece = bt_piecedb_get
};

/** create disk backend */
static void __client_setup_disk_backend(void* bt, unsigned int piece_len)
{
    void* dc, *db;

    dc = bt_diskmem_new();
    bt_diskmem_set_size(dc, piece_len);
    db = bt_piecedb_new();
    bt_piecedb_set_diskstorage(db, bt_diskmem_get_blockrw(dc), NULL, dc);
    bt_piecedb_set_piece_length(db,piece_len);
    bt_client_set_piecedb(bt,&pdb_funcs,db);
}

client_t* client_setup(int log, int id)
{
    client_t* cli;
    void *bt;
    config_t* cfg;

    cli = networkfuns_mock_client_new(id);

    /* bittorrent client */
    cli->bt = bt = bt_client_new();
    cfg = bt_client_get_config(bt);
    config_set(cfg, "my_peerid", bt_generate_peer_id());
    //bt_client_set_peer_id(bt, bt_generate_peer_id());
    //bt_peerconn_set_my_peer_id

    /* set network functions */
    {
        bt_client_funcs_t func = {
            .peer_connect = peer_connect,
            .peer_send =  peer_send,
            .peer_recv_len =peer_recv_len, 
            .peer_disconnect =peer_disconnect, 
            .peers_poll =peers_poll, 
            .peer_listen_open =peer_listen_open
        };

        bt_client_set_funcs(bt, &func, cli);
        bt_client_set_logging(bt,log , __log);
    }

    return cli;
}


void TestBT_Peer_shares_all_pieces(
    CuTest * tc
)
{
    int log;
    int ii;
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;

    network_setup();

    log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(1,5);

    a = client_setup(log, 1);
    b = client_setup(log, 2);

    __client_setup_disk_backend(a->bt,5);
    __client_setup_disk_backend(b->bt,5);

    for (
        hashmap_iterator(__clients, &iter);
        hashmap_iterator_has_next(__clients, &iter);
        )
    {
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(__clients, &iter);
        bt = cli->bt;
        cfg = bt_client_get_config(bt);
        /* default configuration for clients */
        config_set(cfg, "npieces", "1");
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");
        /* add files/pieces */
        bt_piecedb_add_file(bt_client_get_piecedb(bt),"test.txt",8,5);
        bt_piecedb_add(bt_client_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,0));
    }

    /* write blocks to client A */
    {
        void* data;
        bt_block_t blk;

        data = mocktorrent_get_data(mt,0);
        blk.piece_idx = 0;
        blk.block_byte_offset = 0;
        blk.block_len = 5;
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_client_get_piecedb(a->bt)),
                NULL, &blk, data);

        //bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(a->bt));
        CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(a->bt)));
    }

    /* B will initiate the connection */
    bt_client_add_peer(b->bt,NULL,0,"1",1,0);


    for (ii=0; ii<10; ii++)
    {
//        printf("\nStep %d:\n", ii+1);
        bt_client_step(a->bt);
        bt_client_step(b->bt);
        __print_client_contents();
    }

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(b->bt)));
}

void TestBT_Peer_shares_all_pieces_between_each_other(
    CuTest * tc
)
{
    int log;
    int ii;
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;

    network_setup();

    log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(2,5);

    a = client_setup(log, 1);
    b = client_setup(log, 2);

    __client_setup_disk_backend(a->bt,5);
    __client_setup_disk_backend(b->bt,5);

    for (
        hashmap_iterator(__clients, &iter);
        hashmap_iterator_has_next(__clients, &iter);
        )
    {
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(__clients, &iter);
        bt = cli->bt;
        cfg = bt_client_get_config(bt);
        /* default configuration for clients */
        config_set(cfg, "npieces", "2");
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");
        /* add files/pieces */
        bt_piecedb_add_file(bt_client_get_piecedb(bt),"test.txt",8,5);
        bt_piecedb_add_file(bt_client_get_piecedb(bt),"test2.txt",8,12);
        bt_piecedb_add(bt_client_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,0));
        bt_piecedb_add(bt_client_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,1));
    }

    /* write blocks to client A & B */
    {
        void* data;
        bt_block_t blk;

        blk.piece_idx = 0;
        blk.block_byte_offset = 0;
        blk.block_len = 5;

        data = mocktorrent_get_data(mt,0);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_client_get_piecedb(b->bt)),
                NULL, &blk, data);

        blk.piece_idx = 1;
        data = mocktorrent_get_data(mt,1);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_client_get_piecedb(a->bt)),
                NULL, &blk, data);
    }

    /* B will initiate the connection */
    bt_client_add_peer(b->bt,NULL,0,"1",1,0);


    for (ii=0; ii<20; ii++)
    {
//        printf("\nStep %d:\n", ii+1);
        bt_client_step(a->bt);
        bt_client_step(b->bt);
        __print_client_contents();
    }

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(a->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(b->bt)));
}

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
        blk.block_byte_offset = 0;
        blk.block_len = 5;
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
        blk.block_byte_offset = 0;
        blk.block_len = 5;
        data = mocktorrent_get_data(mt,ii);
        bt_diskmem_write_block(bt_piecedb_get_diskstorage(db), NULL, &blk, data);
    }
}

/** a have message has to be sent by client B, for client A and C to finish */
void TestBT_Peer_three_share_all_pieces_between_each_other(
    CuTest * tc
)
{
    int log;
    int ii;
    client_t* a, *b, *c;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;

    network_setup();

    log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(3,5);

    a = client_setup(log, 1);
    b = client_setup(log, 2);
    c = client_setup(log, 3);

    __client_setup_disk_backend(a->bt,5);
    __client_setup_disk_backend(b->bt,5);
    __client_setup_disk_backend(c->bt,5);

    for (
        hashmap_iterator(__clients, &iter);
        hashmap_iterator_has_next(__clients, &iter);
        )
    {
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(__clients, &iter);
        bt = cli->bt;
        cfg = bt_client_get_config(bt);
        /* default configuration for clients */
        config_set(cfg, "npieces", "3");
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");
        /* add files/pieces */
        bt_piecedb_add_file(bt_client_get_piecedb(bt),"test.txt",8,20);
//        bt_piecedb_add_file(bt_client_get_piecedb(bt),"test2.txt",8,12);
        bt_piecedb_add(bt_client_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,0));
        bt_piecedb_add(bt_client_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,1));
        bt_piecedb_add(bt_client_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,2));
    }

    /* write blocks to client A & B */
    {
        void* data;
        bt_block_t blk;

        blk.piece_idx = 0;
        blk.block_byte_offset = 0;
        blk.block_len = 5;

        data = mocktorrent_get_data(mt,0);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_client_get_piecedb(a->bt)),
                NULL, &blk, data);

        blk.piece_idx = 1;
        data = mocktorrent_get_data(mt,1);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_client_get_piecedb(b->bt)),
                NULL, &blk, data);

        blk.piece_idx = 2;
        data = mocktorrent_get_data(mt,2);
        bt_diskmem_write_block(
                bt_piecedb_get_diskstorage(bt_client_get_piecedb(c->bt)),
                NULL, &blk, data);
    }

    /* B will initiate the connection */
    bt_client_add_peer(a->bt,NULL,0,"2",1,0);
    bt_client_add_peer(b->bt,NULL,0,"3",1,0);


    for (ii=0; ii<20; ii++)
    {
//        printf("\nStep %d:\n", ii+1);
        bt_client_step(a->bt);
        bt_client_step(b->bt);
        bt_client_step(c->bt);
        __print_client_contents();
    }

//    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(a->bt));
//    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));
//    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(c->bt));

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(a->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(b->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(c->bt)));
}

void TestBT_Peer_share_100_pieces(
    CuTest * tc
)
{
    int log;
    int ii;
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;

    network_setup();

    log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(100,5);

    a = client_setup(log, 1);
    b = client_setup(log, 2);

    __client_setup_disk_backend(a->bt,5);
    __client_setup_disk_backend(b->bt,5);

    for (
        hashmap_iterator(__clients, &iter);
        hashmap_iterator_has_next(__clients, &iter);
        )
    {
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(__clients, &iter);
        bt = cli->bt;
        cfg = bt_client_get_config(bt);
        /* default configuration for clients */
        config_set(cfg, "npieces", "100");
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");
        /* add files/pieces */
        bt_piecedb_add_file(bt_client_get_piecedb(bt),"test.txt",8,100 * 5);

        for (ii=0; ii<100; ii++)
            bt_piecedb_add(bt_client_get_piecedb(bt),mocktorrent_get_piece_sha1(mt,ii));
    }

  
    /* B will initiate the connection */
    bt_client_add_peer(a->bt,NULL,0,"2",1,0);

    __add_random_piece_subset_of_mocktorrent(
            bt_client_get_piecedb(a->bt),
            mt, 100);

    __add_piece_intersection_of_mocktorrent(
            bt_client_get_piecedb(b->bt),
            bt_client_get_piecedb(a->bt),
            mt, 100);

    for (ii=0; ii<200; ii++)
    {
//        printf("\nStep %d:\n", ii+1);
        bt_client_step(a->bt);
        bt_client_step(b->bt);
//        __print_client_contents();
//        bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));
    }

//    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(a->bt));
//    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(a->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(b->bt)));
}
