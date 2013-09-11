
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

#include "bt_piece_db.h"
#include "bt_diskmem.h"
#include "config.h"
#include "linked_list_hashmap.h"
#include "bipbuffer.h"
#include "networkfuncs_mock.h"

#include <fcntl.h>
#include <sys/time.h>

void *__clients = NULL;


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

void* network_setup()
{
    __clients = hashmap_new(__vptr_hash, __vptr_compare, 11);

    return NULL;
}


static void __log(void *udata, void *src, char *buf)
{
    char stamp[32];
    int fd = (unsigned long) udata;
    struct timeval tv;

#if 0 /* debugging */
    printf(buf);
    printf("\n");

    gettimeofday(&tv, NULL);
    sprintf(stamp, "%d,%0.2f,", (int) tv.tv_sec, (float) tv.tv_usec / 100000);
    write(fd, stamp, strlen(stamp));
    write(fd, buf, strlen(buf));
    write(fd, "\n", 1);
#endif
}

bt_piecedb_i pdb_funcs = {
//    .poll_best_from_bitfield = bt_piecedb_poll_best_from_bitfield,
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
    bt_client_set_piece_db(bt,&pdb_funcs,db);
}

static void __client_add_peer(
    client_t* me,
    char* peer_id,
    unsigned int peer_id_len,
    char* ip,
    unsigned int ip_len,
    unsigned int port)
{
    void* peer;
    void* netdata;
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
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect,
                bt_client_peer_connect_fail))
    {
        printf("failed connection to peer");
    }
#endif

    peer = bt_client_add_peer(me->bt, peer_id, peer_id_len, ip, ip_len, port, peer_nethandle);
}

client_t* client_setup(int log, void* id, int piecelen)
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
            .peer_disconnect =peer_disconnect
        };

        bt_client_set_funcs(bt, &func, cli);
//        bt_client_set_logging(bt,log , __log);
    }

    hashmap_put(__clients, cli, cli);

    __client_setup_disk_backend(cli->bt,piecelen);

    return cli;
}

void TestBT_Peer_shares_all_pieces(
    CuTest * tc
)
{
    int log = 0;
    int ii;
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;
    char *addr;

    network_setup();

//    log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(1,5);

    a = client_setup(log, NULL, 5);
    b = client_setup(log, NULL, 5);

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
        config_set_va(cfg, "piece_length", "%d", 5);
        config_set(cfg, "infohash", "00000000000000000000");
        /* add files/pieces */
        //bt_piecedb_add_file(bt_client_get_piecedb(bt),"test.txt",8,5);
        bt_piecedb_increase_piece_space(bt_client_get_piecedb(bt),5);
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
    asprintf(&addr,"%p", a);
    __client_add_peer(b,NULL,0,addr,strlen(addr),0);
    //bt_client_add_peer(b->bt,NULL,0,addr,strlen(addr),0, a);

    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(a->bt));
    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));

    for (ii=0; ii<10; ii++)
    {
#if 0 /* debugging */
        printf("\nStep %d:\n", ii+1);
#endif
        network_poll(a->bt, &a, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        network_poll(b->bt, &b, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        bt_client_periodic(a->bt);
        bt_client_periodic(b->bt);
//        __print_client_contents();
    }

    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(a->bt));
    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(b->bt)));
}

void TestBT_Peer_shares_all_pieces_between_each_other(
    CuTest * tc
)
{
    int log = 0;
    int ii;
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;
    char *addr;

    network_setup();

    //log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(2,5);

    a = client_setup(log, NULL, 5);
    b = client_setup(log, NULL, 5);

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
        //bt_piecedb_add_file(bt_client_get_piecedb(bt),"test.txt",8,5);
        bt_piecedb_increase_piece_space(bt_client_get_piecedb(bt),5);
        //bt_piecedb_add_file(bt_client_get_piecedb(bt),"test2.txt",8,12);
        bt_piecedb_increase_piece_space(bt_client_get_piecedb(bt),12);
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
    asprintf(&addr,"%p", a);
    __client_add_peer(b,NULL,0,addr,strlen(addr),0);
    //bt_client_add_peer(b->bt,NULL,0,addr,strlen(addr),0, a);


    for (ii=0; ii<20; ii++)
    {
#if 0 /* debugging */
        printf("\nStep %d:\n", ii+1);
#endif
        network_poll(a->bt, &a, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        network_poll(b->bt, &b, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        bt_client_periodic(a->bt);
        bt_client_periodic(b->bt);
//        __print_client_contents();
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
    int log = 0;
    int ii;
    client_t* a, *b, *c;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;
    char *addr;

    network_setup();

    //log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(3,5);

    a = client_setup(log, NULL, 5);
    b = client_setup(log, NULL, 5);
    c = client_setup(log, NULL, 5);

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
        //bt_piecedb_add_file(bt_client_get_piecedb(bt),"test.txt",8,20);
        bt_piecedb_increase_piece_space(bt_client_get_piecedb(bt),20);
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
    asprintf(&addr,"%p", b);
    __client_add_peer(a,NULL,0,addr,strlen(addr),0);
    //bt_client_add_peer(a->bt,NULL,0,addr,strlen(addr),0,b);
    asprintf(&addr,"%p", c);
    __client_add_peer(b,NULL,0,addr,strlen(addr),0);
    //bt_client_add_peer(b->bt,NULL,0,addr,strlen(addr),0,a);


    for (ii=0; ii<20; ii++)
    {
#if 0 /* debugging */
        printf("\nStep %d:\n", ii+1);
#endif
        network_poll(a->bt, &a, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        network_poll(b->bt, &b, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        network_poll(c->bt, &c, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        bt_client_periodic(a->bt);
        bt_client_periodic(b->bt);
        bt_client_periodic(c->bt);
//        __print_client_contents();
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
    int log = 0, num_pieces;
    int ii;
    client_t* a, *b;
    hashmap_iterator_t iter;
    void* mt;
    void* clients;
    char *addr;

    network_setup();

    num_pieces = 100;

    log = 0;
//    log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    mt = mocktorrent_new(num_pieces,5);

    a = client_setup(log, NULL, 5);
    b = client_setup(log, NULL, 5);

    for (
        hashmap_iterator(__clients, &iter);
        hashmap_iterator_has_next(__clients, &iter);
        )
    {
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(__clients, &iter);
        bt = cli->bt;

        /* default configuration for clients */
        cfg = bt_client_get_config(bt);
        config_set_va(cfg, "npieces", "%d", num_pieces);
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");

        /* add files/pieces */
        bt_piecedb_increase_piece_space(bt_client_get_piecedb(bt),num_pieces * 5);
        for (ii=0; ii<num_pieces; ii++)
        {
            void* pd=bt_client_get_piecedb(bt);
            void* sha1=mocktorrent_get_piece_sha1(mt,ii);
            bt_piecedb_add(pd,sha1);
        }
    }
  
    /* B will initiate the connection */
    asprintf(&addr,"%p", b);
    //bt_client_add_peer(a->bt,NULL,0,addr,strlen(addr),0,b);
    __client_add_peer(a,NULL,0,addr,strlen(addr),0);

    __add_random_piece_subset_of_mocktorrent(
            bt_client_get_piecedb(a->bt),
            mt, num_pieces);

    __add_piece_intersection_of_mocktorrent(
            bt_client_get_piecedb(b->bt),
            bt_client_get_piecedb(a->bt),
            mt, num_pieces);

    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(a->bt));
    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));

    for (ii=0; ii<80; ii++)
    {
#if 0 /* debugging */
        printf("\nStep %d:\n", ii+1);
#endif
        network_poll(a->bt, &a, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        network_poll(b->bt, &b, 0,
                bt_client_dispatch_from_buffer,
                bt_client_peer_connect);

        bt_client_periodic(a->bt);
        bt_client_periodic(b->bt);
//        __print_client_contents();
//        bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));
    }

    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(a->bt));
    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(b->bt));

    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(a->bt)));
    CuAssertTrue(tc, 1 == bt_piecedb_all_pieces_are_complete(bt_client_get_piecedb(b->bt)));
}
