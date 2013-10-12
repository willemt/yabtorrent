/**
 * @file
 * @brief Major class tasked with managing downloads
 *        bt_client works similar to the mediator pattern
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
#include "bt_choker_peer.h"
#include "bt_choker.h"
#include "bt_choker_leecher.h"
#include "bt_choker_seeder.h"
#include "bt_selector_random.h"
#include "bt_selector_rarestfirst.h"
#include "bt_selector_sequential.h"

#include "bag.h"

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

    /* network functions */
    bt_client_funcs_t func;
    void *net_udata;

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

    /*  pieces */
    bag_t *p_candidates;

} bt_client_private_t;

/**
 * Peer stats
 * Used for collecting statistics on peers.
 * This is populated by __FUNC_peer_stats_visitor */
typedef struct {
    int failed_connection;
    int connected;
    int peers;
    int choking;
    int download_rate;
    int upload_rate;
} peer_stats_t;

static int __FUNC_peerconn_send_to_peer(void *bto,
                                        const void* pc_peer,
                                        const void *data,
                                        const int len);

static void __log(void *bto, void *src, const char *fmt, ...)
{
    bt_client_private_t *me = bto;
    char buf[1024];
    va_list args;

    if (!me->func_log)
        return;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    me->func_log(me->log_udata, NULL, buf);
}

void __FUNC_peer_periodic(void* caller, void* peer, void* udata)
{
    bt_peer_t* p = peer;

    if (pwp_conn_flag_is_set(p->pc, PC_FAILED_CONNECTION)) return;
    if (!pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED)) return;
    pwp_conn_periodic(p->pc);
}

/**
 * Peer stats visitor
 * Processes each peer connection retrieving stats */
void __FUNC_peer_stats_visitor(void* caller, void* peer, void* udata)
{
    peer_stats_t *stats = udata;
    bt_peer_t* p = peer;

    if (pwp_conn_im_choked(p->pc))
        stats->choking++;
    if (pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED))
        stats->connected++;
    if (pwp_conn_flag_is_set(p->pc, PC_FAILED_CONNECTION))
        stats->failed_connection++;
    stats->download_rate += pwp_conn_get_download_rate(p->pc);
    stats->upload_rate += pwp_conn_get_upload_rate(p->pc);
    stats->peers++;
}

/**
 * Take this PWP message and process it on the Peer Connection side
 * @return 1 on sucess; 0 otherwise
 **/
int bt_client_dispatch_from_buffer(
        void *bto,
        void *peer_nethandle,
        const unsigned char* buf,
        unsigned int len)
{
    bt_client_private_t *me = bto;
    bt_peer_t* peer;

    /* get the peer that this message is for via nethandle */
    peer = bt_peermanager_nethandle_to_peer(me->pm, peer_nethandle);

    /* handle handshake */
    if (!pwp_conn_flag_is_set(peer->pc, PC_HANDSHAKE_RECEIVED))
    {
        int ret;

        ret = pwp_handshaker_dispatch_from_buffer(peer->mh, &buf, &len);

        if (ret == 1)
        {
            /* we're done with handshaking */
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

    /* handle regular PWP traffic */
    pwp_msghandler_dispatch_from_buffer(peer->mh, buf, len);

    return 1;
}

void bt_client_peer_connect_fail(void *bto, void* nethandle)
{
    bt_client_private_t *me = bto;
    bt_peer_t *peer;

    peer = bt_peermanager_nethandle_to_peer(me->pm, nethandle);

    if (!peer)
    {
        return;
    }

    pwp_conn_set_state(peer->pc, PC_FAILED_CONNECTION);
}

void bt_client_peer_connect(void *bto, void* nethandle, char *ip, const int port)
{
    bt_client_private_t *me = bto;
    bt_peer_t *peer;

    peer = bt_peermanager_nethandle_to_peer(me->pm, nethandle);

    /* this is the first time we have come across this peer */
    if (!peer)
    {
        peer = bt_client_add_peer((bt_client_t*)me, "", 0, ip, strlen(ip), port, nethandle);

        if (!peer)
        {
            fprintf(stderr, "cant add peer %s:%d %lx\n",
                    ip, port, (unsigned long int)nethandle);
            return;
        }
    }

    pwp_handshaker_send_handshake(bto, peer,
        __FUNC_peerconn_send_to_peer,
        config_get(me->cfg,"infohash"),
        config_get(me->cfg,"my_peerid"));

//    pwp_conn_send_handshake(peer->pc);
//    __log(bto,NULL,"CONNECTED: peerid:%d ip:%s", netpeerid, ip);
}

static void __log_process_info(bt_client_private_t * me)
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
    bt_client_private_t *me = bto;

    bt_leeching_choker_decide_best_npeers(me->lchoke);
    eventtimer_push_event(me->ticker, 10, me, __leecher_peer_reciprocation);
}

static void __leecher_peer_optimistic_unchoke(void *bto)
{
    bt_client_private_t *me = bto;

    bt_leeching_choker_optimistically_unchoke(me->lchoke);
    eventtimer_push_event(me->ticker, 30, me, __leecher_peer_optimistic_unchoke);
}

static void __FUNC_log(void *bto, void *src, const char *fmt, ...)
{
    bt_client_private_t *me = bto;
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
static int __FUNC_peerconn_send_to_peer(void *bto,
                                        const void* pc_peer,
                                        const void *data,
                                        const int len)
{
    const bt_peer_t * peer = pc_peer;
    bt_client_private_t *me = bto;

    assert(peer);
    assert(me->func.peer_send);
    return me->func.peer_send(me, &me->net_udata, peer->nethandle, data, len);
}

int __fill_bag(bt_client_private_t *me, void* peer)
{
    int idx;
    bt_piece_t* pce;

    while (1)
    {
        idx = me->ips.poll_piece(me->pselector, peer);

        if (idx == -1)
            return -1;

        pce = me->ipdb->get_piece(me->pdb, idx);
        assert(pce);
        if (!bt_piece_is_fully_requested(pce) && !bt_piece_is_complete(pce))
        {
            bag_put(me->p_candidates, (void *) (long) idx + 1);
            break;
        }
    }

    return 0;
}

static int __FUNC_peerconn_pollblock(
        void *bto, void* peer, void* bitfield, bt_block_t * blk)
{
    bitfield_t * peer_bitfield = bitfield;
    bt_client_private_t *me = bto;
    int idx;
    bt_piece_t* pce;

#if 1
    if (bag_count(me->p_candidates) < 3)
    {
        int r;

        r = __fill_bag(me,peer);

        if (-1 == r)
            return r;
    }

    if (0 == bag_count(me->p_candidates))
        return -1;

    while (1)
    {
        idx = ((unsigned long int)bag_take(me->p_candidates)) - 1;
        if (-1 == idx)
        {
            return -1;
        }
        pce = me->ipdb->get_piece(me->pdb, idx);
        assert(pce);
        if (!bt_piece_is_fully_requested(pce))
        {
            bag_put(me->p_candidates, (void *) (long) idx + 1);
            bt_piece_poll_block_request(pce, blk);
            return 0;
        }

        if (bag_count(me->p_candidates) == 0)
            __fill_bag(me,peer);
    }

//    pce = me->ipdb->get_piece(me->pdb, 0);
//    assert(!bt_piece_is_complete(pce));
//    assert(!bt_piece_is_fully_requested(pce));

    return -1;
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
}

static void __FUNC_peerconn_send_have(void* caller, void* peer, void* udata)
{
    bt_peer_t* p = peer;

    if (!pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED)) return;
    pwp_conn_send_have(p->pc, bt_piece_get_idx(udata));
}

static void* __FUNC_get_piece(void* caller, unsigned int idx)
{
    bt_client_private_t *me = caller;
    void* pce;

    pce = me->ipdb->get_piece(me->pdb, idx);
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
    bt_client_private_t *me = bto;
    bt_piece_t *pce;

    assert(me->ipdb->get_piece);

    pce = me->ipdb->get_piece(me->pdb, block->piece_idx);

    assert(pce);

    /* write block to disk medium */
    bt_piece_write_block(pce, NULL, block, data);

    if (bt_piece_is_complete(pce))
    {
//        bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(me));

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

        /* dump everything to disk if the whole download is complete */
        if (bt_piecedb_all_pieces_are_complete(me))
        {
            me->am_seeding = 1;
//            bt_diskcache_disk_dump(me->dc);
        }
#endif

    }

    return 1;
}

void __FUNC_peerconn_log(void *bto, void *src_peer, const char *buf, ...)
{
    bt_client_private_t *me = bto;
    bt_peer_t *peer = src_peer;
    char buffer[256];

    sprintf(buffer, "pwp,%s,%s", peer->peer_id, buf);
    me->func_log(me->log_udata, NULL, buffer);
}

int __FUNC_peerconn_disconnect(void *bto,
        void* pr, char *reason)
{
    bt_client_private_t *me = bto;
    bt_peer_t * peer = pr;

    __log(bto,NULL,"disconnecting,%s", reason);
    bt_client_remove_peer(me,peer);

    return 1;
}

static int __FUNC_peerconn_pieceiscomplete(void *bto, void *piece)
{
    bt_client_private_t *me = bto;
    bt_piece_t *pce = piece;
    int r, status;

    r = bt_piece_is_complete(pce);
    return r;
}

static void __FUNC_peerconn_peer_have_piece(
        void* bt,
        void* peer,
        int idx
        )
{
    bt_client_private_t *me = bt;

    me->ips.peer_have_piece(me->pselector, peer, idx);
}

static void __FUNC_peerconn_giveback_block(
        void* bt,
        void* peer,
        bt_block_t* b
        )
{
    bt_client_private_t *me = bt;
    void* pce;

    if (b->len < 0)
        return;

    pce = me->ipdb->get_piece(me->pdb, b->piece_idx);
    assert(pce);

    bt_piece_giveback_block(pce, b);
    me->ips.peer_giveback_piece(me->pselector, peer, b->piece_idx);
}

static void __FUNC_peerconn_write_block_to_stream(
        void* pce,
        bt_block_t * blk,
        unsigned char ** msg)
{
    bt_piece_write_block_to_stream(pce, blk, msg);
}

pwp_conn_functions_t funcs = {
    .log = __FUNC_log,
    .send = __FUNC_peerconn_send_to_peer,
    .getpiece = __FUNC_get_piece,
    .pushblock = __FUNC_peerconn_pushblock,
    .pollblock = __FUNC_peerconn_pollblock,
    .disconnect = __FUNC_peerconn_disconnect,
    .peer_have_piece = __FUNC_peerconn_peer_have_piece,
    .piece_is_complete = __FUNC_peerconn_pieceiscomplete,
    .write_block_to_stream = __FUNC_peerconn_write_block_to_stream,
    .peer_giveback_block = __FUNC_peerconn_giveback_block,
};

/**
 * Add the peer.
 * Initiate connection with 
 * @return freshly created bt_peer
 */
void *bt_client_add_peer(bt_client_t* me_,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port,
                              void* nethandle)
{
    bt_client_private_t *me = (void*)me_;
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

    if (nethandle)
        peer->nethandle = nethandle;

    void* pc;

    /* create a peer connection for this peer */
    peer->pc = pc = pwp_conn_new();
    pwp_conn_set_functions(pc, &funcs, me);
    pwp_conn_set_piece_info(pc,
            config_get_int(me->cfg,"npieces"),
            config_get_int(me->cfg,"piece_length"));
    pwp_conn_set_peer(pc, peer);

    /* the remote peer will have always send a handshake */
    if (NULL == me->func.peer_connect)
    {
        fprintf(stderr, "cant add peer, peer_connect function not available\n");
        return NULL;
    }

#if 1
    if (!nethandle)
    {
        /* connection */
        if (0 == me->func.peer_connect(me,
                    &me->net_udata,
                    &peer->nethandle,
                    peer->ip,
                    peer->port,
                    bt_client_dispatch_from_buffer,
                    bt_client_peer_connect,
                    bt_client_peer_connect_fail))
        {
            __log(me,NULL,"failed connection to peer");
            return 0;
        }
    }
#endif

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
 * @todo add disconnection functionality
 * @return 1 on sucess; otherwise 0
 */
int bt_client_remove_peer(bt_client_t* me_, void* pr)
{
    bt_client_private_t* me = (void*)me_;
    bt_peer_t* peer = pr;

    bt_peermanager_remove_peer(me->pm,peer);
    me->ips.remove_peer(me->pselector, peer);

    return 1;
}

void bt_client_periodic(bt_client_t* me_)
{
    bt_client_private_t *me = (void*)me_;
    int ii;

    __log_process_info(me);

    /*  shutdown if we are setup to not seed */
    if (1 == me->am_seeding && 1 == config_get_int(me->cfg,"shutdown_when_complete"))
    {
        goto cleanup;
    }

    /*  run each peer connection step */
    bt_peermanager_forall(me->pm,me,NULL,__FUNC_peer_periodic);

    /* TODO: dispatch eventtimer events */

cleanup:

//    bt_piecedb_print_pieces_downloaded(bt_client_get_piecedb(me));
#if 0
    peer_stats_t stat;
    memset(&stat,0,sizeof(peer_stats_t));
    bt_peermanager_forall(me->pm,me,&stat,__FUNC_peer_stats_visitor);
    printf("peers: %d (active:%d choking:%d failed:%d) "
            "downloaded:%d completed:%d/%d dl:%dKB/s ul:%dKB/s\n",
            stat.peers,
            stat.connected,
            stat.choking,
            stat.failed_connection,
            bt_piecedb_get_num_downloaded(bt_client_get_piecedb(me)),
            bt_piecedb_get_num_completed(bt_client_get_piecedb(me)),
            bt_piecedb_get_length(bt_client_get_piecedb(me)),
            stat.download_rate == 0 ? 0 : stat.download_rate / 1000,
            stat.upload_rate == 0 ? 0 : stat.upload_rate / 1000
            );
#endif

    return;
}

void* bt_client_get_config(bt_client_t* me_)
{
    bt_client_private_t *me = (void*)me_;

    return me->cfg;
}

/**
 * Set the logging function
 */
void bt_client_set_logging(bt_client_t* me_, void *udata, func_log_f func)
{
    bt_client_private_t *me = (void*)me_;

    me->func_log = func;
    me->log_udata = udata;
}

/**
 * Set callback functions
 */
void bt_client_set_funcs(bt_client_t* me_, bt_client_funcs_t * func, void* caller)
{
    bt_client_private_t *me = (void*)me_;

    memcpy(&me->func, func, sizeof(bt_client_funcs_t));
    me->net_udata = caller;
}

/**
 * @return number of peers this client is involved with
 */
int bt_client_get_num_peers(bt_client_t* me_)
{
    bt_client_private_t *me = (void*)me_;

    return bt_peermanager_count(me->pm);
}

void *bt_client_get_piecedb(bt_client_t* me_)
{
    bt_client_private_t *me = (void*)me_;

    return me->pdb;
}

/**
 * Set the current piece selector
 * This allows us to use dependency injection to de-couple the
 * implementation of the piece selector from bt_client
 * @param ips Struct of function pointers for piece selector operation
 * @param piece_selector Selector instance. If NULL we call the constructor. */
void bt_client_set_piece_selector(bt_client_t* me_, bt_pieceselector_i* ips, void* piece_selector)
{
    bt_client_private_t* me = (void*)me_;

    memcpy(&me->ips, ips, sizeof(bt_pieceselector_i));

    if (!piece_selector)
    {
        me->pselector = me->ips.new(0);
    }
    else
    {
        me->pselector = piece_selector;
    }
}

/**
 * Initiliase the bittorrent client
 *
 * @return 1 on sucess; otherwise 0
 * \nosubgrouping
 */
void *bt_client_new()
{
    bt_client_private_t *me;

    me = calloc(1, sizeof(bt_client_private_t));

    /* default configuration */
    me->cfg = config_new();
    config_set(me->cfg,"default", "0");
    config_set_if_not_set(me->cfg,"infohash", "00000000000000000000");
    config_set_if_not_set(me->cfg,"my_ip", "127.0.0.1");
    config_set_if_not_set(me->cfg,"pwp_listen_port", "6881");
    config_set_if_not_set(me->cfg,"max_peer_connections", "10");
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

    me->p_candidates = bag_new();

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

    /* Selector */
    bt_pieceselector_i ips = {
        .new = bt_random_selector_new,
        .peer_giveback_piece = bt_random_selector_giveback_piece,
        .have_piece = bt_random_selector_have_piece,
        .remove_peer = bt_random_selector_remove_peer,
        .add_peer = bt_random_selector_add_peer,
        .peer_have_piece = bt_random_selector_peer_have_piece,
        .get_npeers = bt_random_selector_get_npeers,
        .get_npieces = bt_random_selector_get_npieces,
        .poll_piece = bt_random_selector_poll_best_piece
    };

    bt_client_set_piece_selector((void*)me, &ips, NULL);

    return me;
}


void bt_client_set_piece_db(bt_client_t* me_, bt_piecedb_i* ipdb, void* piece_db)
{
    bt_client_private_t* me = (void*)me_;

    me->ipdb = ipdb;
    me->pdb = piece_db;
}

void *bt_peer_get_nethandle(void* pr)
{
    bt_peer_t* peer = pr;

    return peer->nethandle;
}

/**
 * Release all memory used by the client
 * Close all peer connections
 * @todo add destructors
 */
int bt_client_release(bt_client_t* me_)
{
    return 1;
}

