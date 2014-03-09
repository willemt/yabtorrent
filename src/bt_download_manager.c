/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Major class tasked with managing downloads
 *        bt_dm works similar to the mediator pattern
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

/* for varags */
#include <stdarg.h>

#include "bitfield.h"
#include "event_timer.h"
#include "config.h"
#include "linked_list_queue.h"
#include "sparse_counter.h"

#include "pwp_connection.h"
#include "pwp_msghandler.h"

#include "bt.h"
#include "bt_peermanager.h"
#include "bt_string.h"
#include "bt_piece_db.h"
#include "bt_piece.h"
#include "bt_blacklist.h"
#include "bt_choker_peer.h"
#include "bt_choker.h"
#include "bt_choker_leecher.h"
#include "bt_choker_seeder.h"
#include "bt_selector_random.h"
#include "bt_selector_rarestfirst.h"
#include "bt_selector_sequential.h"

#include <time.h>

typedef struct
{
    /* database for writing pieces */
    bt_piecedb_i ipdb;
    bt_piecedb_t* pdb;

    /* callbacks */
    bt_dm_cbs_t cb;

    /* callback context */
    void *cb_ctx;

    /* job management */
    void *job_lock;
    linked_list_queue_t *jobs;

    /* configuration */
    void* cfg;

    /* peer manager */
    void* pm;

    /* peer and piece blacklisting */
    void* blacklist;

    /*  leeching choker */
    void *lchoke;

    /* timer */
    void *ticker;

    /* for selecting pieces */
    bt_pieceselector_i ips;
    void* pselector;

    /* are we seeding? */
    int am_seeding;

    sparsecounter_t* pieces_completed;

} bt_dm_private_t;

typedef struct {
    bt_peer_t* peer;
    bt_block_t blk;
} bt_job_pollblock_t;

typedef struct {
    bt_peer_t* peer;
    int piece_idx;
} bt_job_validate_piece_t;

enum {
    BT_JOB_NONE,
    BT_JOB_POLLBLOCK,
    BT_JOB_VALIDATE_PIECE
};

typedef struct {
    int type;
    union {
        bt_job_pollblock_t pollblock;
        bt_job_validate_piece_t validate_piece;
    };
} bt_job_t;


void *bt_dm_get_piecedb(bt_dm_t* me_);

static void __log(void *me_, void *src, const char *fmt, ...)
{
    bt_dm_private_t *me = me_;
    char buf[1024], *p;
    va_list args;

    if (!me->cb.log)
        return;

    p = buf;
    if (config_get(me->cfg,"my_peerid"))
        sprintf(p, "%s,", config_get(me->cfg,"my_peerid"));
        p += strlen(buf);

    va_start(args, fmt);
    vsprintf(p, fmt, args);
    me->cb.log(me->cb_ctx, NULL, buf);
}

static int __FUNC_peerconn_send_to_peer(void *me_,
                                        const void* pc_peer,
                                        const void *data,
                                        const int len);

int __FUNC_peerconn_disconnect(void *me_, void* pr, char *reason);

void __FUNC_peer_periodic(void* cb_ctx, void* peer, void* udata)
{
    bt_peer_t* p = peer;

    if (pwp_conn_flag_is_set(p->pc, PC_FAILED_CONNECTION)) return;
    if (!pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED)) return;
    pwp_conn_periodic(p->pc);
}

void __FUNC_peer_stats_visitor(void* cb_ctx, void* peer, void* udata)
{
    bt_dm_stats_t *s = udata;
    bt_dm_peer_stats_t *ps; 
    bt_peer_t* p = peer;

    ps = &s->peers[s->npeers++];
    ps->choked = pwp_conn_im_choked(p->pc);
    ps->choking = pwp_conn_im_choking(p->pc);
    ps->connected = pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED);
    ps->failed_connection = pwp_conn_flag_is_set(p->pc, PC_FAILED_CONNECTION);
    ps->drate = pwp_conn_get_download_rate(p->pc);
    ps->urate = pwp_conn_get_upload_rate(p->pc);
}

/**
 * @return 0 if handshake not complete, 1 otherwise */
static int __handle_handshake(
    bt_dm_private_t *me,
    bt_peer_t* p,
    const unsigned char** buf,
    int *len)
{
    /* TODO: needs test case */
    if (!me->cb.handshaker_dispatch_from_buffer)
    {
        /* If funcs.handshaker_dispatch_from_buffer is not set:
         *  No handshaking will occur. This is useful for client variants which
         *  establish a handshake earlier at a higher level. */
        pwp_conn_set_state(p->pc, PC_HANDSHAKE_RECEIVED);
        return 1;
    }

    switch (me->cb.handshaker_dispatch_from_buffer(p->mh, buf, len))
    {
    case BT_HANDSHAKER_DISPATCH_SUCCESS:
        __log(me, NULL, "handshake,successful, 0x%lx", (unsigned long)p->pc);
        me->cb.handshaker_release(p->mh);
        p->mh = me->cb.msghandler_new(me->cb_ctx, p->pc);
        pwp_conn_set_state(p->pc, PC_HANDSHAKE_RECEIVED);
        if (me->cb.handshake_success)
            me->cb.handshake_success((void*)me, me->cb_ctx, p->pc, p->conn_ctx);
        return 1;
    default:
    case BT_HANDSHAKER_DISPATCH_REMAINING:
        return 0;
    case BT_HANDSHAKER_DISPATCH_ERROR:
        assert(0);
        __log(me, NULL, "handshake,failed");
        return 0;
    }
}

/* TODO: needs test cases */
int bt_dm_dispatch_from_buffer(
        void *me_,
        void *peer_conn_ctx,
        const unsigned char* buf,
        unsigned int len)
{
    bt_dm_private_t *me = me_;
    bt_peer_t* p;

    if (!(p = bt_peermanager_conn_ctx_to_peer(me->pm, peer_conn_ctx)))
    {
        return 0;
    }

    if (!pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED))
    {
        switch (__handle_handshake(me,p,&buf,&len))
        {
            case 1: break;
            case 0: /* error */ return 1;
        }
    }

    switch (pwp_msghandler_dispatch_from_buffer(p->mh, buf, len))
    {
    case 1: /* successful */
        break;
    case 0: /* error, we need to disconnect */
        __FUNC_peerconn_disconnect(me_, p, "bad msg detected by PWP handler");
        break;
    }

    return 1;
}

void bt_dm_peer_connect_fail(void *me_, void* conn_ctx)
{
    bt_dm_private_t *me = me_;
    bt_peer_t *peer;

    if (!(peer = bt_peermanager_conn_ctx_to_peer(me->pm, conn_ctx)))
    {
        return;
    }

    pwp_conn_set_state(peer->pc, PC_FAILED_CONNECTION);
}

int bt_dm_peer_connect(void *me_, void* conn_ctx, char *ip, const int port)
{
    bt_dm_private_t *me = me_;
    bt_peer_t *peer;

    /* this is the first time we have come across this peer */
    if (!(peer = bt_peermanager_conn_ctx_to_peer(me->pm, conn_ctx)))
    {
        return 0;
    }

    if (me->cb.send_handshake)
        me->cb.send_handshake(me_, peer,
            __FUNC_peerconn_send_to_peer,
            config_get(me->cfg,"infohash"),
            config_get(me->cfg,"my_peerid"));
    return 1;
}

static int __get_drate(const void *me_, const void *pc)
{
    return pwp_conn_get_download_rate(pc);
}

static int __get_urate(const void *me_, const void *pc)
{
    return pwp_conn_get_upload_rate(pc);
}

static int __get_is_interested(void *me_, void *pc)
{
    return pwp_conn_peer_is_interested(pc);
}

static void __choke_peer(void *me_, void *pc)
{
    pwp_conn_choke_peer(pc);
}

static void __unchoke_peer(void *me_, void *pc)
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

static void __leecher_peer_reciprocation(void *me_)
{
    bt_dm_private_t *me = me_;
    bt_leeching_choker_decide_best_npeers(me->lchoke);
    eventtimer_push_event(me->ticker, 10, me, __leecher_peer_reciprocation);
}

static void __leecher_peer_optimistic_unchoke(void *me_)
{
    bt_dm_private_t *me = me_;
    bt_leeching_choker_optimistically_unchoke(me->lchoke);
    eventtimer_push_event(me->ticker, 30, me, __leecher_peer_optimistic_unchoke);
}

/**
 * Peer connections are given this as a callback whenever they want to send
 * information */
static int __FUNC_peerconn_send_to_peer(void *me_,
                                        const void* pc_peer,
                                        const void *data,
                                        const int len)
{
    const bt_peer_t * peer = pc_peer;
    bt_dm_private_t *me = me_;

    assert(peer);
    assert(me->cb.peer_send);
    return me->cb.peer_send(me, &me->cb_ctx, peer->conn_ctx, data, len);
}

static void __FUNC_peerconn_send_have(void* cb_ctx, void* peer, void* udata)
{
    bt_peer_t* p = peer;

    if (!pwp_conn_flag_is_set(p->pc, PC_HANDSHAKE_RECEIVED)) return;
    pwp_conn_send_have(p->pc, bt_piece_get_idx(udata));
}

static void* __offer_job(void *me_, void* j_)
{
    bt_dm_private_t* me = me_;
    llqueue_offer(me->jobs, j_);

    /* unused */
    return NULL; 
}

static void* __poll_job(void *me_, void* __unused)
{
    bt_dm_private_t* me = me_;
    return llqueue_poll(me->jobs);
}

static void __job_dispatch_poll_piece(bt_dm_private_t* me, bt_job_t* j)
{
    assert(me->ips.poll_piece);

    while (1)
    {
        int p_idx = me->ips.poll_piece(me->pselector, j->pollblock.peer);

        if (-1 == p_idx)
            break;

        bt_piece_t* pce = me->ipdb.get_piece(me->pdb, p_idx);

        if (pce && bt_piece_is_complete(pce))
        {
            me->ips.have_piece(me->pselector, p_idx);
            continue;
        }

        while (!bt_piece_is_fully_requested(pce))
        {
            bt_block_t blk;
            bt_piece_poll_block_request(pce, &blk);
            pwp_conn_offer_block(j->pollblock.peer->pc, &blk);
        }

        break;
    }
}

static void __job_dispatch_validate_piece(bt_dm_private_t* me, bt_job_t* j)
{
    bt_piece_t *p;
    int piece_idx;

    p = me->ipdb.get_piece(me->pdb, j->validate_piece.piece_idx);
    piece_idx = bt_piece_get_idx(p);

    switch (bt_piece_validate(p))
    {
    case BT_PIECE_VALIDATE_COMPLETE_PIECE:
    {
        __log(me, NULL, "client,piece completed,pieceidx=%d", piece_idx);
        assert(me->ips.have_piece);
        me->ips.have_piece(me->pselector, piece_idx);
        sc_mark_complete(me->pieces_completed, piece_idx, 1);
        bt_peermanager_forall(me->pm,me,p,__FUNC_peerconn_send_have);
    }
        break;

    case BT_PIECE_VALIDATE_ERROR: /* error */
        break;

    case BT_PIECE_VALIDATE_INVALID_PIECE: /* invalid piece */
    {
        /* only peer involved in piece download, therefore treat as
         * untrusted and blacklist */
        if (1 == bt_piece_num_peers(p))
        {
            int i = 0;
            void* peer;

            peer = bt_piece_get_peers(p,&i);
            bt_blacklist_add_peer(me->blacklist,p,peer);
        }
        else 
        {
            int i = 0;
            void* p2;

            for (p2 = bt_piece_get_peers(p,&i); p2;
                    p2 = bt_piece_get_peers(p,&i))
            {
                bt_blacklist_add_peer_as_potentially_blacklisted(
                        me->blacklist, p, p2);
            }

            bt_piece_drop_download_progress(p);
            me->ips.peer_giveback_piece(me->pselector, NULL, p->idx);
        }
    }
        break;
    }
}

static void __dispatch_job(bt_dm_private_t* me, bt_job_t* j)
{
    assert(j);

    switch (j->type)
    {
    case BT_JOB_POLLBLOCK: __job_dispatch_poll_piece(me,j); break;
    case BT_JOB_VALIDATE_PIECE: __job_dispatch_validate_piece(me,j); break;
    default: assert(0); break;
    }

    /* TODO: replace with mempool */
    free(j);
}

static void* __call_exclusively(void* me_, void** lock, void *j, 
    void* (*func)(void *me_, void* j_))
{
    bt_dm_private_t *me = me_;

    assert(func);

    if (me->cb.call_exclusively)
        return me->cb.call_exclusively(me_, me->cb_ctx, lock, j, func);
    else return func(me_,j);
}

static int __FUNC_peerconn_pollblock(void *me_, void* peer)
{
    bt_dm_private_t *me = me_;
    bt_job_t* j;

    /* TODO: replace malloc() with memory pool/arena */
    j = malloc(sizeof(bt_job_t));
    j->type = BT_JOB_POLLBLOCK;
    j->pollblock.peer = peer;
    __call_exclusively(me, &me->job_lock, j, __offer_job);
    return 0;
}

/**
 * Received a block from a peer
 * @param peer Peer received from
 * @param data Data to be pushed */
int __FUNC_peerconn_pushblock(
        void *me_,
        void* pr,
        bt_block_t *b,
        const void *data)
{
    bt_peer_t *peer = pr;
    bt_dm_private_t *me = me_;
    bt_piece_t *p;

    assert(me->ipdb.get_piece);

    p = me->ipdb.get_piece(me->pdb, b->piece_idx);

    switch (bt_piece_write_block(p, NULL, b, data, peer))
    {
    case BT_PIECE_WRITE_BLOCK_COMPLETELY_DOWNLOADED:
    {
        /* TODO: replace malloc() with memory pool/arena */
        bt_job_t * j = malloc(sizeof(bt_job_t));
        j->type = BT_JOB_VALIDATE_PIECE;
        j->validate_piece.peer = peer;
        j->validate_piece.piece_idx = b->piece_idx;
        __call_exclusively(me, &me->job_lock, j, __offer_job);
    }
    break;
    case BT_PIECE_WRITE_BLOCK_SUCCESS: break;
    case 0:
        printf("error writing block\n");
        break;
    }

    return 1;
}

void __FUNC_peerconn_log(void *me_, void *src_peer, const char *buf, ...)
{
    bt_peer_t *peer = src_peer;
    char buffer[1000];

    sprintf(buffer, "pwp,%s,%s", peer->peer_id, buf);
    __log(me_,NULL,buffer);
}

int __FUNC_peerconn_disconnect(void *me_, void* pr, char *reason)
{
    bt_peer_t * peer = pr;

    __log(me_,NULL,"disconnecting,%s", reason);
    bt_dm_remove_peer(me_,peer);
    return 1;
}

static void __FUNC_peerconn_peer_have_piece(void* bt, void* peer, int idx)
{
    bt_dm_private_t *me = bt;
    me->ips.peer_have_piece(me->pselector, peer, idx);
}

static void __FUNC_peerconn_giveback_block(void* bt, void* peer, bt_block_t* b)
{
    bt_dm_private_t *me = bt;
    void* pce;

    if (b->len < 0)
        return;

    pce = me->ipdb.get_piece(me->pdb, b->piece_idx);
    bt_piece_giveback_block(pce, b);
    me->ips.peer_giveback_piece(me->pselector, peer, b->piece_idx);
}

static void __FUNC_peerconn_write_block_to_stream(void* cb_ctx,
        bt_block_t * blk,
        unsigned char ** msg)
{
    bt_dm_private_t *me = cb_ctx;
    void* p;

    if (!(p = me->ipdb.get_piece(me->pdb, blk->piece_idx)))
    {
        __log(me,NULL,"ERROR,unable to obtain piece");
        return;
    }

    if (0 == bt_piece_write_block_to_stream(p, blk, msg))
    {
        __log(me,NULL,"ERROR,unable to write block to stream");
    }
}

void *bt_dm_add_peer(bt_dm_t* me_,
                      const char *peer_id,
                      const int peer_id_len,
                      const char *ip, const int ip_len, const int port,
                      void* conn_ctx,
                      void* conn_mem)
{
    bt_dm_private_t *me = (void*)me_;
    bt_peer_t* p;

    /*  ensure we aren't adding ourselves as a peer */
    if (!strncmp(ip, config_get(me->cfg,"my_ip"), ip_len) &&
            port == atoi(config_get(me->cfg,"pwp_listen_port")))
    {
        return NULL;
    }

    /* remember the peer */
    if (!(p = bt_peermanager_add_peer(me->pm, peer_id, peer_id_len,
                    ip, ip_len, port)))
    {
#if 0 /* debug */
        fprintf(stderr, "cant add %s:%d, it's been added already\n", ip, port);
#endif
        return NULL;
    }

    if (me->pselector)
        me->ips.add_peer(me->pselector, p);

    if (conn_ctx)
        p->conn_ctx = conn_ctx;

    void* pc = p->pc = pwp_conn_new(conn_mem);
    pwp_conn_set_cbs(pc, &((pwp_conn_cbs_t) {
        .log = __FUNC_peerconn_log,
        .send = __FUNC_peerconn_send_to_peer,
        .pushblock = __FUNC_peerconn_pushblock,
        .pollblock = __FUNC_peerconn_pollblock,
        .disconnect = __FUNC_peerconn_disconnect,
        .peer_have_piece = __FUNC_peerconn_peer_have_piece,
        .peer_giveback_block = __FUNC_peerconn_giveback_block,
        .write_block_to_stream = __FUNC_peerconn_write_block_to_stream,
        .call_exclusively = me->cb.call_exclusively
        }), me);
    pwp_conn_set_progress(pc, me->pieces_completed);
    pwp_conn_set_piece_info(pc,
            config_get_int(me->cfg,"npieces"),
            config_get_int(me->cfg,"piece_length"));
    pwp_conn_set_peer(pc, p);

    __log(me,NULL,"added peer %.*s:%d 0x%lx",
            ip_len, ip, port, (unsigned long)pc);

    if (!conn_ctx && me->cb.peer_connect)
    {
        if (0 == me->cb.peer_connect(me,
                    &me->cb_ctx,
                    &p->conn_ctx,
                    p->ip,
                    p->port,
                    bt_dm_dispatch_from_buffer,
                    bt_dm_peer_connect,
                    bt_dm_peer_connect_fail))
        {
            __log(me, NULL, "failed connection to peer");
            return NULL;
        }
    }

    if (me->cb.handshaker_new)
        p->mh = me->cb.handshaker_new(
                config_get(me->cfg, "infohash"),
                config_get(me->cfg, "my_peerid"));

    bt_leeching_choker_add_peer(me->lchoke, p->pc);

    return p;
}

int bt_dm_remove_peer(bt_dm_t* me_, void* pr)
{
    bt_dm_private_t* me = (void*)me_;
    bt_peer_t* peer = pr;

    if (0 == bt_peermanager_remove_peer(me->pm,peer))
    {
        __log(me_,NULL,"ERROR,couldn't remove peer");
        return 0;
    }
    me->ips.remove_peer(me->pselector, peer);
    return 1;
}

int bt_dm_get_jobs(bt_dm_t* me_)
{
    bt_dm_private_t *me = (void*)me_;
    return llqueue_count(me->jobs);
}

void bt_dm_periodic(bt_dm_t* me_, bt_dm_stats_t *stats)
{
    bt_dm_private_t *me = (void*)me_;

    bt_peermanager_forall(me->pm, me, NULL, __FUNC_peer_periodic);

    /* TODO: pump out keep alive message */

    while (0 < llqueue_count(me->jobs))
    {
        void *j = __call_exclusively(me_, &me->job_lock, NULL, __poll_job);
        __dispatch_job(me,j);
    }

    if (1 == me->am_seeding
            && 1 == config_get_int(me->cfg, "shutdown_when_complete"))
    {
        goto cleanup;
    }

    /* TODO: dispatch eventtimer events */

cleanup:

    if (stats)
    {
        /* enlarge stats array */
        if (stats->npeers_size < bt_peermanager_count(me->pm))
        {
            stats->npeers_size = bt_peermanager_count(me->pm);
            stats->peers = realloc(stats->peers,
                    stats->npeers_size * sizeof(bt_dm_peer_stats_t));
        }
        stats->npeers = 0;
        bt_peermanager_forall(me->pm,me,stats,__FUNC_peer_stats_visitor);
    }

    return;
}

static void* __default_msghandler_new(void* callee, void* pc)
{
    return pwp_msghandler_new(pc);
}

static void __default_handshake_success(
        void* me_,
        void* udata,
        void* pc,
        void* p_conn_ctx)
{
    bt_dm_private_t *me = (void*)me_;
    bt_peer_t* p = bt_peermanager_conn_ctx_to_peer(me->pm, p_conn_ctx);

    if (0 == pwp_send_bitfield(config_get_int(me->cfg, "npieces"),
            me->pieces_completed, __FUNC_peerconn_send_to_peer, me, p))
    {
        __FUNC_peerconn_disconnect((void*)me, p, "couldn't send bitfield");
    }
}

void bt_dm_set_cbs(bt_dm_t* me_, bt_dm_cbs_t * func, void* cb_ctx)
{
    bt_dm_private_t *me = (void*)me_;

    me->cb_ctx = cb_ctx;
    memcpy(&me->cb, func, sizeof(bt_dm_cbs_t));
    if (!me->cb.msghandler_new)
        me->cb.msghandler_new = __default_msghandler_new;
    if (!me->cb.handshake_success)
        me->cb.handshake_success = __default_handshake_success;
}

void* bt_dm_get_config(bt_dm_t* me_)
{
    bt_dm_private_t *me = (void*)me_;
    return me->cfg;
}

int bt_dm_get_num_peers(bt_dm_t* me_)
{
    bt_dm_private_t *me = (void*)me_;
    return bt_peermanager_count(me->pm);
}

void *bt_dm_get_piecedb(bt_dm_t* me_)
{
    bt_dm_private_t *me = (void*)me_;
    return me->pdb;
}

void bt_dm_set_piece_selector(
        bt_dm_t* me_,
        bt_pieceselector_i* ips,
        void* piece_selector)
{
    bt_dm_private_t* me = (void*)me_;

    memcpy(&me->ips, ips, sizeof(bt_pieceselector_i));
    if (!piece_selector)
        me->pselector = me->ips.new(0);
    else
        me->pselector = piece_selector;
    bt_dm_check_pieces(me_);
}

void bt_dm_set_piece_db(bt_dm_t* me_, bt_piecedb_i* ipdb, void* piece_db)
{
    bt_dm_private_t* me = (void*)me_;
    memcpy(&me->ipdb,ipdb,sizeof(bt_piecedb_i));
    me->pdb = piece_db;
}

void bt_dm_check_pieces(bt_dm_t* me_)
{
    bt_dm_private_t* me = (void*)me_;
    int i, end;

    for (i=0, end = config_get_int(me->cfg,"npieces"); i<end; i++)
    {
        bt_piece_t* p = me->ipdb.get_piece(me->pdb, i);

        if (!p) continue;
        if (!bt_piece_is_complete(p))
        {
            bt_job_t * j = malloc(sizeof(bt_job_t));
            j->type = BT_JOB_VALIDATE_PIECE;
            j->validate_piece.peer = NULL;
            j->validate_piece.piece_idx = bt_piece_get_idx(p);
            __call_exclusively(me, &me->job_lock, j, __offer_job);
        }
    }
}

int bt_dm_release(bt_dm_t* me_)
{
    /* TODO add destructors */
    return 1;
}

void *bt_dm_new()
{
    bt_dm_private_t *me;

    me = calloc(1, sizeof(bt_dm_private_t));
    me->jobs = llqueue_new();
    me->job_lock = NULL;
    me->blacklist = bt_blacklist_new();
    me->pm = bt_peermanager_new(me);
    bt_peermanager_set_config(me->pm, me->cfg);

    /* default configuration */
    me->cfg = config_new();
    config_set(me->cfg,"default", "0");
    config_set_if_not_set(me->cfg,"infohash", "00000000000000000000");
    config_set_if_not_set(me->cfg,"my_ip", "127.0.0.1");
    config_set_if_not_set(me->cfg,"pwp_listen_port", "6881");
    config_set_if_not_set(me->cfg,"max_peer_connections", "32");
    config_set_if_not_set(me->cfg,"max_active_peers", "32");
    config_set_if_not_set(me->cfg,"max_pending_requests", "10");
    config_set_if_not_set(me->cfg,"npieces", "0");
    config_set_if_not_set(me->cfg,"piece_length", "0");
    config_set_if_not_set(me->cfg,"download_path", ".");
    config_set_if_not_set(me->cfg,"shutdown_when_complete", "0");

    /*  set leeching choker */
    me->lchoke = bt_leeching_choker_new(
            atoi(config_get(me->cfg,"max_active_peers")));
    bt_leeching_choker_set_choker_peer_iface(me->lchoke, me,
            &iface_choker_peer);

    /* timing */
    me->ticker = eventtimer_new();
    eventtimer_push_event(me->ticker, 10, me, __leecher_peer_reciprocation);
    eventtimer_push_event(me->ticker, 30, me, __leecher_peer_optimistic_unchoke);

    /* we don't need to specify the amount of pieces we need */
    me->pieces_completed = sc_init(0);
    return me;
}

void *bt_peer_get_conn_ctx(void* pr)
{
    bt_peer_t* peer = pr;
    return peer->conn_ctx;
}
