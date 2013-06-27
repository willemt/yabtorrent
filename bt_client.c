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

/* --------------------------------------------------------------------------*/

static void __log(void *bto, void *src, const char *fmt, ...)
{
    bt_client_t *bt = bto;
    char buf[1024];
    va_list args;

    if (!bt->func_log)
        return;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    bt->func_log(bt->log_udata, NULL, buf);
}

void __FUNC_peer_step(void* caller, void* peer, void* udata)
{
    bt_peer_t* p = peer;

    if (!pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED)) return;

    pwp_conn_step(p->pc);
}

/**
 * Take this message and process it on the PeerConnection side
 * */
static int __process_peer_msg(
        void *bto,
        const int netpeerid,
        const unsigned char* buf,
        unsigned int len)
{
    bt_client_t *bt = bto;
    bt_peer_t* peer;

    /* get the peer that this message is for using the netpeerid*/
    peer = bt_peermanager_netpeerid_to_peer(bt->pm, netpeerid);

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
            assert(0);
        }
    }

    pwp_msghandler_dispatch_from_buffer(peer->mh, buf, len);

    return 1;
}

static void __process_peer_connect(void *bto,
                                   const int netpeerid,
                                   char *ip, const int port)
{
    bt_client_t *bt = bto;
    bt_peer_t *peer;
    void *pc;

    peer = bt_client_add_peer(bt, NULL, 0, ip, strlen(ip), port);
    peer->net_peerid = netpeerid;
    pwp_conn_connected(peer->pc);
    pwp_conn_send_handshake(peer->pc);
    __log(bto,NULL,"CONNECTED: peerid:%d ip:%s", netpeerid, ip);
}

static void __log_process_info(bt_client_t * bt)
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
    bt_client_t *bt = bto;

    bt_leeching_choker_decide_best_npeers(bt->lchoke);
    eventtimer_push_event(bt->ticker, 10, bt, __leecher_peer_reciprocation);
}

static void __leecher_peer_optimistic_unchoke(void *bto)
{
    bt_client_t *bt = bto;

    bt_leeching_choker_optimistically_unchoke(bt->lchoke);
    eventtimer_push_event(bt->ticker, 30, bt, __leecher_peer_optimistic_unchoke);
}

/**
 * Initiliase the bittorrent client
 *
 * @return 1 on sucess; otherwise 0
 *
 * \nosubgrouping
 */
void *bt_client_new()
{
    bt_client_t *bt;

    bt = calloc(1, sizeof(bt_client_t));

    /* default configuration */
    bt->cfg = config_new();
    config_set(bt->cfg,"default", "0");
    config_set_if_not_set(bt->cfg,"infohash", "00000000000000000000");
    config_set_if_not_set(bt->cfg,"my_ip", "127.0.0.1");
    config_set_if_not_set(bt->cfg,"pwp_listen_port", "6000");
    config_set_if_not_set(bt->cfg,"max_peer_connections", "10");
    config_set_if_not_set(bt->cfg,"select_timeout_msec", "1000");
    config_set_if_not_set(bt->cfg,"max_active_peers", "4");
    config_set_if_not_set(bt->cfg,"tracker_scrape_interval", "10");
    config_set_if_not_set(bt->cfg,"max_pending_requests", "10");
    /* How many pieces are there of this file
     * The size of a piece is determined by the publisher of the torrent.
     * A good recommendation is to use a piece size so that the metainfo file does
     * not exceed 70 kilobytes.  */
    config_set_if_not_set(bt->cfg,"npieces", "10");
    config_set_if_not_set(bt->cfg,"piece_length", "10");
    config_set_if_not_set(bt->cfg,"download_path", ".");
    /* Set maximum amount of megabytes used by piece cache */
    config_set_if_not_set(bt->cfg,"max_cache_mem_bytes", "1000000");
    /* If this is set, the client will shutdown when the download is completed. */
    config_set_if_not_set(bt->cfg,"shutdown_when_complete", "0");

    /* need to be able to tell the time */
    bt->ticker = eventtimer_new();

    /* peer manager */
    bt->pm = bt_peermanager_new(bt);
    bt_peermanager_set_config(bt->pm,bt->cfg);

    /*  set leeching choker */
    bt->lchoke = bt_leeching_choker_new(atoi(config_get(bt->cfg,"max_active_peers")));
    bt_leeching_choker_set_choker_peer_iface(bt->lchoke, bt,
                                             &iface_choker_peer);

    /*  start reciprocation timer */
    eventtimer_push_event(bt->ticker, 10, bt, __leecher_peer_reciprocation);

    /*  start optimistic unchoker timer */
    eventtimer_push_event(bt->ticker, 30, bt, __leecher_peer_optimistic_unchoke);

    return bt;
}

void bt_client_set_piecedb(void* bto, bt_piecedb_i* ipdb, void* piecedb)
{
    bt_client_t* bt = bto;

    bt->ipdb = ipdb;
    bt->piecedb = piecedb;
}

/**
 * Release all memory used by the client
 * Close all peer connections
 * @todo add destructors
 */
int bt_client_release(void *bto)
{
    //FIXME_STUB;

    return 1;
}

static void __FUNC_log(void *bto, void *src, const char *fmt, ...)
{
    bt_client_t *bt = bto;
    char buf[1024], *p;
    va_list args;

    p = buf;

    if (!bt->func_log)
        return;

    sprintf(p, "%s,", config_get(bt->cfg,"my_peerid"));

    p += strlen(buf);

    va_start(args, fmt);
    vsprintf(p, fmt, args);
//    printf("%s\n", buf);
    bt->func_log(bt->log_udata, NULL, buf);
}

/**
 * Peer connections are given this as a callback whenever they want to send
 * information */
int __FUNC_peerconn_send_to_peer(void *bto,
                                        const void* pc,
                                        const void *data,
                                        const int len)
{
    const bt_peer_t * peer = pc;
    bt_client_t *bt = bto;

//    peer = bt_peermanager_get_peer_from_pc(bt->pm,pc);
    assert(pc);
    assert(peer);
    assert(bt->func.peer_send);
    return bt->func.peer_send(&bt->net_udata, peer->net_peerid, data, len);
}

int __FUNC_peerconn_pollblock(void *bto,
        void* bitfield, bt_block_t * blk)
{

    bitfield_t * peer_bitfield = bitfield;
    bt_client_t *bt = bto;
    bt_piece_t *pce;

    if ((pce = bt->ipdb->poll_best_from_bitfield(bt->piecedb, peer_bitfield)))
    {
        assert(pce);
        assert(!bt_piece_is_complete(pce));
        assert(!bt_piece_is_fully_requested(pce));
        bt_piece_poll_block_request(pce, blk);
        return 0;
    }
    else
    {
        return -1;
    }
}

static void __FUNC_peerconn_send_have(void* caller, void* peer, void* udata)
{
    bt_peer_t* p = peer;

    pwp_conn_send_have(p->pc, bt_piece_get_idx(udata));
}

static void* __FUNC_get_piece(void* caller, unsigned int idx)
{
    bt_client_t *bt = caller;

    return bt->ipdb->get_piece(bt->piecedb, idx);
}

/**
 * Received a block from a peer
 *
 * @param peer : peer received from
 * */
int __FUNC_peerconn_pushblock(void *bto,
                                    void* pr,
                                    bt_block_t * block,
                                    const void *data)
{
    bt_peer_t * peer = pr;
    bt_client_t *bt = bto;
    bt_piece_t *pce;

    pce = bt->ipdb->get_piece(bt->piecedb, block->piece_idx);

    assert(pce);

    /* write block to disk medium */
    bt_piece_write_block(pce, NULL, block, data);
//    bt_filedumper_write_block(bt->fd, block, data);

    if (bt_piece_is_complete(pce))
    {
        /*  send have to all peers */
        if (bt_piece_is_valid(pce))
        {
            int ii;

            __log(bt, NULL, "client,piece downloaded,pieceidx=%d",
                  bt_piece_get_idx(pce));

            /* send HAVE messages to all peers */
            bt_peermanager_forall(bt->pm,bt,pce,__FUNC_peerconn_send_have);
        }

#if 0
        bt_piecedb_print_pieces_downloaded(bt->db);

        /* dump everything to disk if the whole download is complete */
        if (bt_piecedb_all_pieces_are_complete(bt))
        {
            bt->am_seeding = 1;
//            bt_diskcache_disk_dump(bt->dc);
        }
#endif

    }

    return 1;
}

void __FUNC_peerconn_log(void *bto, void *src_peer, const char *buf, ...)
{
    bt_client_t *bt = bto;

    bt_peer_t *peer = src_peer;

    char buffer[256];

    sprintf(buffer, "pwp,%s,%s", peer->peer_id, buf);
    bt->func_log(bt->log_udata, NULL, buffer);
}

int __FUNC_peerconn_disconnect(void *bto,
        void* pr, char *reason)
{
    bt_peer_t * peer = pr;
    __log(bto,NULL,"disconnecting,%s", reason);
    return 1;
}

int __FUNC_peerconn_pieceiscomplete(void *bto, void *piece)
{
    bt_client_t *bt = bto;
    bt_piece_t *pce = piece;

    return bt_piece_is_complete(pce);
}

void __FUNC_peerconn_write_block_to_stream(void* pce, bt_block_t * blk, byte ** msg)
{
    bt_piece_write_block_to_stream(pce, blk, msg);
}

pwp_connection_functions_t funcs = {
    .send = __FUNC_peerconn_send_to_peer,
    .pushblock = __FUNC_peerconn_pushblock,
    .pollblock = __FUNC_peerconn_pollblock,
    .disconnect = __FUNC_peerconn_disconnect,
//    .connect = __FUNC_peerconn_connect,
    .getpiece = __FUNC_get_piece,
    .write_block_to_stream = __FUNC_peerconn_write_block_to_stream,
    .piece_is_complete = __FUNC_peerconn_pieceiscomplete,
    .log = __FUNC_log
};

/*---------------------------------------------------------------------------*/

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
        return NULL;
    }

    {
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
            return 0;

        /* connection */
        if (0 == me->func.peer_connect(&me->net_udata, peer->ip,
                                      peer->port, &peer->net_peerid))
        {
            __log(me,NULL,"failed connection to peer");
            return 0;
        }
    }

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
    return 1;
}

/**
 * Tell us if the download is done.
 * @return 1 on sucess; otherwise 0
 */
int bt_client_is_done(void *bto)
{
    return 0;
}

/**
 * Used for stepping the bittorrent logic
 * @return 1 on sucess; otherwise 0
 */
int bt_client_step(void *bto)
{
    bt_client_t *bt = bto;
    int ii;

    __log_process_info(bt);

    /*  shutdown if we are setup to not seed */
    if (1 == bt->am_seeding && 1 == config_get_int(bt->cfg,"shutdown_when_complete"))
    {
        return 0;
    }

    /*  poll data from peer pwp connections */
    bt->func.peers_poll(&bt->net_udata,
                       atoi(config_get(bt->cfg,"select_timeout_msec")),
                       __process_peer_msg,
                       __process_peer_connect, bt);

    /*  run each peer connection step */
    bt_peermanager_forall(bt->pm,NULL,NULL,__FUNC_peer_step);

//    bt_piecedb_print_pieces_downloaded(bt->db);
//    if (__all_pieces_are_complete(bt))
//        return 0;

    return 1;
}

/**
 * Used for initiation of downloading
 */
void bt_client_go(void *bto)
{
    bt_client_t *bt = bto;
    int ii;

    bt->func.peer_listen_open(&bt->net_udata,
            atoi(config_get(bt->cfg,"pwp_listen_port")));

    while (1)
    {
        if (0 == bt_client_step(bt))
            break;
    }

    __log(bt, NULL, "download is done");

    //__dumppiece(bt);
}

