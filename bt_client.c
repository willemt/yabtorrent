/**
 * @file
 * @brief Major class tasked with managing downloads
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include <sys/time.h>

#include <stdarg.h>

#include <pthread.h>

#include "bitfield.h"
#include "pwp_connection.h"
#include "pwp_handshaker.h"
#include "pwp_msghandler.h"

#include "event_timer.h"
#include "config.h"

#include "bt.h"
#include "bt_local.h"
#include "bt_peermanager.h"
#include "bt_block_readwriter_i.h"
#include "bt_string.h"

#include "bt_client_private.h"
#include "bt_choker_peer.h"
#include "bt_choker.h"
#include "bt_choker_leecher.h"
#include "bt_choker_seeder.h"
#include "bt_selector_random.h"
#include "bt_selector_rarestfirst.h"
#include "bt_selector_sequential.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

/*
 * bt_client
 *  has a pwp_conn_t for each peer_t
 *
 * peerconnection_t has a peer_t
 *  downloads from peer
 *  manages connection with peer
 *  adds blocks to piece_t
 *
 * piecedb_t
 *  stores pieces
 *  decides which piece is best
 *
 * piece_t
 *  manage progress of self
 *
 * filedumper
 *  maps pieces to files
 *  dumps files to disk
 *  reads files from disk
 */

typedef struct
{
    /* database for writing pieces */
    bt_piecedb_i *ipdb;
    void* pdb;

    /* net stuff */
    bt_client_funcs_t func;
    void *net_udata;

    char fail_reason[255];

    /*  are we seeding? */
    int am_seeding;

    /*  logging  */
    func_log_f func_log;
    void *log_udata;

    /* configuration */
    void* cfg;

    /* peer manager */
    void* pm;

    /*  leeching choker */
    void *lchoke;

    /* timer */
    void *ticker;

    /* for selecting pieces */
    bt_pieceselector_i ips;
    void* pselector;

    pthread_mutex_t mtx;

} bt_client_t;

static void __log(void *bto, void *src, const char *fmt, ...)
{
    bt_client_t *me = bto;
    char buf[1024];
    va_list args;

    if (!me->func_log)
        return;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    me->func_log(me->log_udata, NULL, buf);
}

void __FUNC_peer_step(void* caller, void* peer, void* udata)
{
    bt_peer_t* p = peer;

//    if (!pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED)) return;
    pwp_conn_periodic(p->pc);
}

/**
 * Take this message and process it on the PeerConnection side
 **/
static int __process_peer_msg(
        void *bto,
        void* nethandle,
        const unsigned char* buf,
        unsigned int len)
{
    bt_client_t *me = bto;
    bt_peer_t* peer;

    /* get the peer that this message is for using the nethandle*/
    peer = bt_peermanager_nethandle_to_peer(me->pm, nethandle);

    /* handle handshake */
    if (!pwp_conn_flag_is_set(peer->pc, PC_HANDSHAKE_RECEIVED))
    {
        int ret;

        ret = pwp_handshaker_dispatch_from_buffer(peer->mh, &buf, &len);

        if (ret == 1)
        {
            pwp_handshaker_release(peer->mh);
            peer->mh = pwp_msghandler_new(peer->pc);
            pwp_conn_set_state(peer->pc, PC_HANDSHAKE_RECEIVED);
            pwp_conn_send_bitfield(peer->pc);
        }
        else if (ret == -1)
        {
            printf("ERROR: bad handshake\n");
            return 0;
        }
    }

    pwp_msghandler_dispatch_from_buffer(peer->mh, buf, len);

    return 1;
}

static void __process_peer_connect_fail(void *bto, void* nethandle)
{
    bt_client_t *me = bto;

    printf("failed connection\n");
}

static void __process_peer_connect(void *bto,
                                   void* nethandle,
                                   char *ip, const int port)
{
    bt_client_t *me = bto;
    bt_peer_t *peer;

//    pthread_mutex_lock(&me->mtx);

    peer = bt_peermanager_nethandle_to_peer(me->pm, nethandle);

    /* this is the first time we have come across this peer */
    if (!peer)
    {
        peer = bt_client_add_peer(me, "", 0, ip, strlen(ip), port);

        if (!peer)
        {
            fprintf(stderr, "cant add peer %s:%d %lx\n", ip, port, nethandle);
            goto cleanup;
        }
    }

    pwp_conn_send_handshake(peer->pc);
//    __log(bto,NULL,"CONNECTED: peerid:%d ip:%s", netpeerid, ip);

cleanup:
    return;
//    pthread_mutex_unlock(&me->mtx);
}

static void __log_process_info(bt_client_t * me)
{
    static long int last_run = 0;
    struct timeval tv;

#define SECONDS_SINCE_LAST_LOG 1
    gettimeofday(&tv, NULL);

    /*  run every n seconds */
    if (0 == last_run)
    {
        last_run = tv.tv_usec;
    }
    else
    {
        unsigned int diff = abs(last_run - tv.tv_usec);

        if (diff >= SECONDS_SINCE_LAST_LOG)
            return;
        last_run = tv.tv_usec;
    }
}

static int __get_drate(const void *bto, const void *pc)
{
//    return pwp_conn_get_download_rate(pc);
    return 0;
}

static int __get_urate(const void *bto, const void *pc)
{
//    return pwp_conn_get_upload_rate(pc);
    return 0;
}

static int __get_is_interested(void *bto, void *pc)
{
    return pwp_conn_peer_is_interested(pc);
}

static void __choke_peer(void *bto, void *pc)
{
    pwp_conn_choke_peer(pc);
}

static void __unchoke_peer(void *bto, void *pc)
{
    pwp_conn_unchoke_peer(pc);
}

static bt_choker_peer_i iface_choker_peer = {
    .get_drate = __get_drate,
    .get_urate = __get_urate,
    .get_is_interested = __get_is_interested,
    .choke_peer = __choke_peer,
    .unchoke_peer = __unchoke_peer
};

static void __leecher_peer_reciprocation(void *bto)
{
    bt_client_t *me = bto;

    bt_leeching_choker_decide_best_npeers(me->lchoke);
    eventtimer_push_event(me->ticker, 10, me, __leecher_peer_reciprocation);
}

static void __leecher_peer_optimistic_unchoke(void *bto)
{
    bt_client_t *me = bto;

    bt_leeching_choker_optimistically_unchoke(me->lchoke);
    eventtimer_push_event(me->ticker, 30, me, __leecher_peer_optimistic_unchoke);
}

/**
 * Initiliase the bittorrent client
 *
 * @return 1 on sucess; otherwise 0
 * \nosubgrouping
 */
void *bt_client_new()
{
    bt_client_t *me;

    me = calloc(1, sizeof(bt_client_t));

    /* default configuration */
    me->cfg = config_new();
    config_set(me->cfg,"default", "0");
    config_set_if_not_set(me->cfg,"infohash", "00000000000000000000");
    config_set_if_not_set(me->cfg,"my_ip", "127.0.0.1");
    config_set_if_not_set(me->cfg,"pwp_listen_port", "6881");
    config_set_if_not_set(me->cfg,"max_peer_connections", "10");
    config_set_if_not_set(me->cfg,"select_timeout_msec", "1000");
    config_set_if_not_set(me->cfg,"max_active_peers", "4");
    config_set_if_not_set(me->cfg,"tracker_scrape_interval", "10");
    config_set_if_not_set(me->cfg,"max_pending_requests", "10");
    /* How many pieces are there of this file
     * The size of a piece is determined by the publisher of the torrent.
     * A good recommendation is to use a piece size so that the metainfo file does
     * not exceed 70 kilobytes.  */
    config_set_if_not_set(me->cfg,"npieces", "10");
    config_set_if_not_set(me->cfg,"piece_length", "10");
    config_set_if_not_set(me->cfg,"download_path", ".");
    /* Set maximum amount of megabytes used by piece cache */
    config_set_if_not_set(me->cfg,"max_cache_mem_bytes", "1000000");
    /* If this is set, the client will shutdown when the download is completed. */
    config_set_if_not_set(me->cfg,"shutdown_when_complete", "0");

    /* need to be able to tell the time */
    me->ticker = eventtimer_new();

    /* peer manager */
    me->pm = bt_peermanager_new(me);
    bt_peermanager_set_config(me->pm, me->cfg);

    /*  set leeching choker */
    me->lchoke = bt_leeching_choker_new(atoi(config_get(me->cfg,"max_active_peers")));
    bt_leeching_choker_set_choker_peer_iface(me->lchoke, me,
                                             &iface_choker_peer);
    /*  start reciprocation timer */
    eventtimer_push_event(me->ticker, 10, me, __leecher_peer_reciprocation);

    /*  start optimistic unchoker timer */
    eventtimer_push_event(me->ticker, 30, me, __leecher_peer_optimistic_unchoke);

    int r;

    if (0 != (r = pthread_mutex_init(&me->mtx, NULL)))
    {
        printf("error: %s\n", strerror(r));
    }

    assert(r==0);

    /* Selector */
#if 0
    me->ips.new = bt_rarestfirst_selector_new,
    me->ips.offer_piece = bt_rarestfirst_selector_offer_piece,
    me->ips.have_piece = bt_rarestfirst_selector_have_piece,
    me->ips.remove_peer = bt_rarestfirst_selector_remove_peer,
    me->ips.add_peer = bt_rarestfirst_selector_add_peer,
    me->ips.peer_have_piece = bt_rarestfirst_selector_peer_have_piece,
    me->ips.get_npeers = bt_rarestfirst_selector_get_npeers,
    me->ips.get_npieces = bt_rarestfirst_selector_get_npieces,
    me->ips.poll_piece = bt_rarestfirst_selector_poll_best_piece
#else
    me->ips.new = bt_random_selector_new;
    me->ips.peer_giveback_piece = bt_random_selector_giveback_piece;
    me->ips.have_piece = bt_random_selector_have_piece;
    me->ips.remove_peer = bt_random_selector_remove_peer;
    me->ips.add_peer = bt_random_selector_add_peer;
    me->ips.peer_have_piece = bt_random_selector_peer_have_piece;
    me->ips.get_npeers = bt_random_selector_get_npeers;
    me->ips.get_npieces = bt_random_selector_get_npieces;
    me->ips.poll_piece = bt_random_selector_poll_best_piece;
    /*
    me->ips.new = bt_sequential_selector_new;
    me->ips.offer_piece = bt_sequential_selector_offer_piece;
    me->ips.have_piece = bt_sequential_selector_have_piece;
    me->ips.remove_peer = bt_sequential_selector_remove_peer;
    me->ips.add_peer = bt_sequential_selector_add_peer;
    me->ips.peer_have_piece = bt_sequential_selector_peer_have_piece;
    me->ips.get_npeers = bt_sequential_selector_get_npeers;
    me->ips.get_npieces = bt_sequential_selector_get_npieces;
    me->ips.poll_piece = bt_sequential_selector_poll_best_piece;
    */
#endif
    me->pselector = me->ips.new(0);

    return me;
}

void bt_client_set_piece_selector(void* bto, bt_pieceselector_i* ips, void* piece_selector)
{
    bt_client_t* me = bto;

    memcpy(&me->ips, ips, sizeof(bt_pieceselector_i));
    me->pselector = piece_selector;
}

void bt_client_set_piece_db(void* bto, bt_piecedb_i* ipdb, void* piece_db)
{
    bt_client_t* me = bto;

    me->ipdb = ipdb;
    me->pdb = piece_db;
}

/**
 * Release all memory used by the client
 * Close all peer connections
 * @todo add destructors
 */
int bt_client_release(void *bto)
{
    return 1;
}

static void __FUNC_log(void *bto, void *src, const char *fmt, ...)
{
    bt_client_t *me = bto;
    char buf[1024], *p;
    va_list args;

    p = buf;

    if (!me->func_log)
        return;

    sprintf(p, "%s,", config_get(me->cfg,"my_peerid"));
    p += strlen(buf);

    va_start(args, fmt);
    vsprintf(p, fmt, args);
    me->func_log(me->log_udata, NULL, buf);
}

/**
 * Peer connections are given this as a callback whenever they want to send
 * information */
int __FUNC_peerconn_send_to_peer(void *bto,
                                        const void* pc_peer,
                                        const void *data,
                                        const int len)
{
    const bt_peer_t * peer = pc_peer;
    bt_client_t *me = bto;

    assert(peer);
    assert(me->func.peer_send);
    return me->func.peer_send(me, &me->net_udata, peer->nethandle, data, len);
}

static int __FUNC_peerconn_pollblock(
        void *bto, void* peer, void* bitfield, bt_block_t * blk)
{
    bitfield_t * peer_bitfield = bitfield;
    bt_client_t *me = bto;
    int idx;
    bt_piece_t* pce;

#if 1
//    pthread_mutex_lock(&me->mtx);

    while (1)
    {
        idx = me->ips.poll_piece(me->pselector, peer);

        if (idx == -1)
            return -1;

        pce = me->ipdb->get_piece(me->pdb, idx);
        assert(pce);
        if (!bt_piece_is_fully_requested(pce) && !bt_piece_is_complete(pce))
        {
            break;
        }
    }

    assert(!bt_piece_is_complete(pce));
    assert(!bt_piece_is_fully_requested(pce));
    bt_piece_poll_block_request(pce, blk);

    return 0;
#else
    int r;

    if ((pce = me->ipdb->poll_best_from_bitfield(me->pdb, peer_bitfield)))
    {
        assert(pce);
        assert(!bt_piece_is_complete(pce));
        assert(!bt_piece_is_fully_requested(pce));
        bt_piece_poll_block_request(pce, blk);
        r = 0;
    }
    else
    {
        r = -1;
    }

    return r;
#endif

//    pthread_mutex_unlock(&me->mtx);
}

static void __FUNC_peerconn_send_have(void* caller, void* peer, void* udata)
{
    bt_peer_t* p = peer;

    pwp_conn_send_have(p->pc, bt_piece_get_idx(udata));
}

static void* __FUNC_get_piece(void* caller, unsigned int idx)
{
    bt_client_t *me = caller;
    void* pce;

//    pthread_mutex_lock(&me->mtx);

    pce = me->ipdb->get_piece(me->pdb, idx);

//    pthread_mutex_unlock(&me->mtx);

    return pce;
}

/**
 * Received a block from a peer
 * @param peer Peer received from
 * @param data Data to be pushed
 * */
int __FUNC_peerconn_pushblock(void *bto,
                                    void* pr,
                                    bt_block_t * block,
                                    const void *data)
{
    bt_peer_t * peer = pr;
    bt_client_t *me = bto;
    bt_piece_t *pce;

//    pthread_mutex_lock(&me->mtx);

    assert(me->ipdb->get_piece);
    pce = me->ipdb->get_piece(me->pdb, block->piece_idx);

    assert(pce);

    /* write block to disk medium */
    bt_piece_write_block(pce, NULL, block, data);
    if (bt_piece_is_complete(pce))
    {
        /*  send have to all peers */
        if (bt_piece_is_valid(pce))
        {
            int ii;

            assert(me->ips.have_piece);
            me->ips.have_piece(me->pselector, block->piece_idx);

            __log(me, NULL, "client,piece downloaded,pieceidx=%d",
                  bt_piece_get_idx(pce));

            /* send HAVE messages to all peers */
            bt_peermanager_forall(me->pm,me,pce,__FUNC_peerconn_send_have);
        }

#if 0
        bt_piecedb_print_pieces_downloaded(me->db);

        /* dump everything to disk if the whole download is complete */
        if (bt_piecedb_all_pieces_are_complete(me))
        {
            me->am_seeding = 1;
//            bt_diskcache_disk_dump(me->dc);
        }
#endif

    }

//    pthread_mutex_unlock(&me->mtx);

    return 1;
}

void __FUNC_peerconn_log(void *bto, void *src_peer, const char *buf, ...)
{
    bt_client_t *me = bto;

    bt_peer_t *peer = src_peer;

    char buffer[256];

    sprintf(buffer, "pwp,%s,%s", peer->peer_id, buf);
    me->func_log(me->log_udata, NULL, buffer);
}

int __FUNC_peerconn_disconnect(void *bto,
        void* pr, char *reason)
{
    bt_peer_t * peer = pr;
    __log(bto,NULL,"disconnecting,%s", reason);
    return 1;
}

static int __FUNC_peerconn_pieceiscomplete(void *bto, void *piece)
{
    bt_client_t *me = bto;
    bt_piece_t *pce = piece;
    int r, status;

//    if (0 != (status = pthread_mutex_lock(&me->mtx))) {
//        printf("error: %s\n", strerror(status));
//    }

    r = bt_piece_is_complete(pce);
//    pthread_mutex_unlock(&me->mtx);
    return r;
}

static void __FUNC_peerconn_peer_have_piece(
        void* bt,
        void* peer,
        int idx
        )
{
    bt_client_t *me = bt;

    me->ips.peer_have_piece(me->pselector, peer, idx);
}

static void __FUNC_peerconn_giveback_piece(
        void* bt,
        void* peer,
        int idx
        )
{
    bt_client_t *me = bt;

    me->ips.peer_giveback_piece(me->pselector, peer, idx);
}

static void __FUNC_peerconn_write_block_to_stream(
        void* pce,
        bt_block_t * blk,
        unsigned char ** msg)
{
    bt_piece_write_block_to_stream(pce, blk, msg);
}

pwp_connection_functions_t funcs = {
    .log = __FUNC_log,
    .send = __FUNC_peerconn_send_to_peer,
    .getpiece = __FUNC_get_piece,
    .pushblock = __FUNC_peerconn_pushblock,
    .pollblock = __FUNC_peerconn_pollblock,
    .disconnect = __FUNC_peerconn_disconnect,
    .peer_have_piece = __FUNC_peerconn_peer_have_piece,
    .piece_is_complete = __FUNC_peerconn_pieceiscomplete,
    .write_block_to_stream = __FUNC_peerconn_write_block_to_stream,
    .peer_giveback_piece = __FUNC_peerconn_peer_have_piece,
};

/**
 * Add the peer.
 * Initiate connection with 
 * @return freshly created bt_peer
 */
void *bt_client_add_peer(void *bto,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port)
{
    bt_client_t *me = bto;
    bt_peer_t* peer;

    /*  ensure we aren't adding ourselves as a peer */
    if (!strncmp(ip, config_get(me->cfg,"my_ip"), ip_len) &&
            port == atoi(config_get(me->cfg,"pwp_listen_port")))
    {
        return NULL;
    }

    /* remember the peer */
    if (!(peer = bt_peermanager_add_peer(me->pm, peer_id, peer_id_len, ip, ip_len, port)))
    {
        fprintf(stderr, "cant add peer %s:%d as it has been added already\n", ip, port);
        return NULL;
    }
    else
    {
        me->ips.add_peer(me->pselector, peer);
    }

    void* pc;

    /* create a peer connection for this peer */
    peer->pc = pc = pwp_conn_new();
    pwp_conn_set_functions(pc, &funcs, me);
    pwp_conn_set_piece_info(pc,
            config_get_int(me->cfg,"npieces"),
            config_get_int(me->cfg,"piece_length"));
    pwp_conn_set_peer(pc, peer);
    pwp_conn_set_infohash(pc, config_get(me->cfg,"infohash"));
    pwp_conn_set_my_peer_id(pc, config_get(me->cfg,"my_peerid"));;
    pwp_conn_set_their_peer_id(pc, strdup(peer->peer_id));

    /* the remote peer will have always send a handshake */
    if (NULL == me->func.peer_connect)
    {
        fprintf(stderr, "cant add peer, peer_connect function not available\n");
        return NULL;
    }

    /* connection */
    if (0 == me->func.peer_connect(me,
                &me->net_udata,
                peer->ip,
                peer->port,
                &peer->nethandle,
                __process_peer_msg,
                __process_peer_connect,
                __process_peer_connect_fail))
    {
        __log(me,NULL,"failed connection to peer");
        return 0;
    }

    /* create handshaker */
    peer->mh = pwp_handshaker_new(
            config_get(me->cfg,"infohash"),
            config_get(me->cfg,"my_peerid"));

    bt_leeching_choker_add_peer(me->lchoke, peer->pc);

    return peer;
}

/**
 * Remove the peer.
 * Disconnect the peer
 *
 * @todo add disconnection functionality
 *
 * @return 1 on sucess; otherwise 0
 */
int bt_client_remove_peer(void *bto, const int peerid)
{

//    me->ips.remove_peer(me->pselector, peer);

    return 1;
}

/**
 * Used for stepping the bittorrent logic
 * @return 1 on sucess; otherwise 0
 */
int bt_client_step(void *bto)
{
#if 1
    bt_client_t *me = bto;
    int ii;

    __log_process_info(me);

    /*  shutdown if we are setup to not seed */
    if (1 == me->am_seeding && 1 == config_get_int(me->cfg,"shutdown_when_complete"))
    {
        return 0;
    }

    /*  poll data from peer pwp connections */
    me->func.peers_poll(me, &me->net_udata,
                       atoi(config_get(me->cfg,"select_timeout_msec")),
                       __process_peer_msg,
                       __process_peer_connect);

    /*  run each peer connection step */
    bt_peermanager_forall(me->pm,NULL,NULL,__FUNC_peer_step);
#endif
    return 1;
}

void bt_client_periodic(void* bto)
{
    bt_client_t *me = bto;
    int ii;

//    pthread_mutex_lock(&me->mtx);

    __log_process_info(me);

    /*  shutdown if we are setup to not seed */
    if (1 == me->am_seeding && 1 == config_get_int(me->cfg,"shutdown_when_complete"))
    {
        goto cleanup;
    }

    /*  run each peer connection step */
    bt_peermanager_forall(me->pm,NULL,NULL,__FUNC_peer_step);

    /* TODO: dispatch eventtimer events */

cleanup:
//    pthread_mutex_unlock(&me->mtx);
    return;
}

void* bt_client_get_config(void *bto)
{
    bt_client_t *me = bto;

    return me->cfg;
}

/**
 * Set the logging function
 */
void bt_client_set_logging(void *bto, void *udata, func_log_f func)
{
    bt_client_t *me = bto;

    me->func_log = func;
    me->log_udata = udata;
}

/**
 * Set callback functions
 */
void bt_client_set_funcs(void *bto, bt_client_funcs_t * func, void* caller)
{
    bt_client_t *me = bto;

    memcpy(&me->func, func, sizeof(bt_client_funcs_t));
    me->net_udata = caller;
}

/**
 * @return number of peers this client is involved with
 */
int bt_client_get_num_peers(void *bto)
{
    bt_client_t *me = bto;

    return bt_peermanager_count(me->pm);
}

/**
 * @return reason for failure
 */
char *bt_client_get_fail_reason(void *bto)
{
    bt_client_t *me = bto;

    return me->fail_reason;
}

/**
 * @return number of bytes downloaded
 */
int bt_client_get_nbytes_downloaded(void *bto)
{
    //FIXME_STUB;
    return 0;
}

/**
 * @todo implement
 * @return 1 if client has failed 
 */
int bt_client_is_failed(void *bto)
{
    return 0;
}

void *bt_client_get_piecedb(void *bto)
{
    bt_client_t *me = bto;

    return me->pdb;
}

