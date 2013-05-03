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

#include "pwp_connection.h"

#include "bitfield.h"
#include "event_timer.h"
#include "config.h"

#include "bt.h"
#include "bt_local.h"
#include "bt_peermanager.h"
#include "bt_block_readwriter_i.h"
#include "bt_filedumper.h"
#include "bt_diskcache.h"
#include "bt_piece_db.h"
#include "bt_string.h"

#include "bt_client_private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

/*
 * bt_client
 *  has a bt_peerconn_t for each peer_t
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

void __FUNC_peerconn_step(void* caller, void* peer, void* udata)
{
    bt_peerconn_step(peer);
}

/**
 * Take this message and process it on the PeerConnection side
 * */
static int __process_peer_msg(void *bto, const int netpeerid)
{
    bt_client_t *bt = bto;
    void *pc;

    pc = bt_peermanager_netpeerid_to_peerconn(bt->pm, netpeerid);
    bt_peerconn_process_msg(pc);
    return 1;
}

static void __process_peer_connect(void *bto,
                                   const int netpeerid,
                                   char *ip, const int port)
{
    bt_client_t *bt = bto;
    bt_peer_t *peer;

    peer = bt_client_add_peer(bt, NULL, 0, ip, strlen(ip), port);
    peer->net_peerid = netpeerid;
//    pc = __netpeerid_to_peerconn(bt, netpeerid);
}

static void __log_process_info(bt_client_t * bt)
{
#if 0
    struct rusage
    {
        struct timeval ru_utime;        /* user time used */
        struct timeval ru_stime;        /* system time used */
        long ru_maxrss;         /* maximum resident set size */
        long ru_ixrss;          /* integral shared memory size */
        long ru_idrss;          /* integral unshared data size */
        long ru_isrss;          /* integral unshared stack size */
        long ru_minflt;         /* page reclaims */
        long ru_majflt;         /* page faults */
        long ru_nswap;          /* swaps */
        long ru_inblock;        /* block input operations */
        long ru_oublock;        /* block output operations */
        long ru_msgsnd;         /* messages sent */
        long ru_msgrcv;         /* messages received */
        long ru_nsignals;       /* signals received */
        long ru_nvcsw;          /* voluntary context switches */
        long ru_nivcsw;         /* involuntary context switches */
    };
#endif
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

#if 0
    struct rusage stats;
    getrusage(RUSAGE_SELF, &stats);
    __log(bt, NULL,
          "stats, ru_utime=%d, ru_stime=%d, ru_maxrss=%d, ru_ixrss=%d, ru_idrss=%d, ru_isrss=%d, ru_minflt=%d, ru_majflt=%d, ru_nswap=%d, ru_inblock=%d, ru_oublock=%d, ru_msgsnd=%d, ru_msgrcv=%d, ru_nsignals=%d, ru_nvcsw=%d, ru_nivcsw=%d",
          stats.ru_utime, stats.ru_stime, stats.ru_maxrss,
          stats.ru_ixrss, stats.ru_idrss, stats.ru_isrss,
          stats.ru_minflt, stats.ru_majflt,
          stats.ru_nswap, stats.ru_inblock,
          stats.ru_oublock, stats.ru_msgsnd,
          stats.ru_msgrcv, stats.ru_nsignals, stats.ru_nvcsw, stats.ru_nivcsw);
#endif
}


static int __get_drate(const void *bto, const void *pc)
{
    return bt_peerconn_get_download_rate(pc);
}

static int __get_urate(const void *bto, const void *pc)
{
    return bt_peerconn_get_upload_rate(pc);
}

static int __get_is_interested(void *bto, void *pc)
{
    return bt_peerconn_peer_is_interested(pc);
}

static void __choke_peer(void *bto, void *pc)
{
    bt_peerconn_choke(pc);
}

static void __unchoke_peer(void *bto, void *pc)
{
    bt_peerconn_unchoke(pc);
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

    /* database for dumping pieces to disk */
    bt->fd = bt_filedumper_new();

    /* intermediary between filedumper and DB */
    bt->dc = bt_diskcache_new();
    bt_diskcache_set_func_log(bt->dc, __log, bt);
    /* point diskcache to filedumper */
    bt_diskcache_set_disk_blockrw(bt->dc,
                                  bt_filedumper_get_blockrw(bt->fd), bt->fd);

    /* database for writing pieces */
    bt->db = bt_piecedb_new();
    /* point piece database to diskcache */
    bt_piecedb_set_diskstorage(bt->db,
                               bt_diskcache_get_blockrw(bt->dc), bt->dc);



    /* peer manager */
    bt->pm = bt_peermanager_new();
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

/**
 * Read metainfo file (ie. "torrent" file)
 * This function will populate the piece database
 * @return 1 on sucess; otherwise 0
 */
int bt_client_read_metainfo_file(void *bto, const char *fname)
{
    bt_client_t *bt = bto;
    char *contents;
    int len;

    if (!(contents = read_file(fname, &len)))
    {
        __log(bto,NULL,"ERROR: torrent file: %s can't be read\n", fname);
        return 0;
    }

    //bt_client_read_metainfo(bto, contents, len, &bt->cfg.pinfo);

    /*  find out if we are seeding by default */
    bt_piecedb_print_pieces_downloaded(bt->db);
    if (bt_piecedb_all_pieces_are_complete(bt->db))
    {
        bt->am_seeding = 1;
    }

    free(contents);

    return 1;
}

/**
 * Obtain this piece from the piece database
 * @return piece specified by piece_idx; otherwise NULL
 */
void *bt_client_get_piece(void *bto, const unsigned int piece_idx)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get(bt->db, piece_idx);
}

/**
 * Mass add pieces to the piece database
 * @param pieces A string of 20 byte sha1 hashes. Is always a multiple of 20 bytes in length. 
 * @param bto the bittorrent client object
 * */
void bt_client_add_pieces(void *bto, const char *pieces, int len)
{
    bt_client_t *bt = bto;

    bt_piecedb_add_all(bt->db, pieces, len);

    /* remember how many pieces there are now */
    config_set_va(bt->cfg,"npieces","%d",bt_piecedb_get_length(bt->db));
}

/**
 * Add this file to the bittorrent client
 * This is used for adding new files.
 *
 * @param id id of bt client
 * @param fname file name
 * @param fname_len length of fname
 * @param flen length in bytes of the file
 * @return 1 on sucess; otherwise 0
 */
int bt_client_add_file(void *bto,
                       const char *fname, const int fname_len, const int flen)
{
    bt_client_t *bt = bto;
    char *path = NULL;

    /* tell the piece DB how big the file is */
    bt_piecedb_set_tot_file_size(bt->db, bt_piecedb_get_tot_file_size(bt->db) + flen);

    /* add the file to the filedumper */
    asprintf(&path, "%.*s", fname_len, fname);
    bt_filedumper_add_file(bt->fd, path, flen);
    return 1;
}

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

    peer = bt_peermanager_add_peer(me->pm,peer_id, peer_id_len, ip, ip_len, port);

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
    bt->net.peers_poll(&bt->net_udata,
                       atoi(config_get(bt->cfg,"select_timeout_msec")),
                       __process_peer_msg, __process_peer_connect, bt);

    /*  run each peer connection step */
    bt_peermanager_forall(bt->pm,NULL,NULL,__FUNC_peerconn_step);

//    if (__all_pieces_are_complete(bt))
//        return 0;
    return 1;
}

#if 0
static void __dumppiece(bt_client_t* bt)
{
    int fd;
    fd = open("piecedump", O_CREAT | O_WRONLY);
    for (ii = 0; ii < bt_piecedb_get_length(bt->db); ii++)
    {
        bt_piece_t *pce;
        
        pce = bt_piecedb_get(bt->db, ii);
        write(fd, (void*)bt_piece_get_data(pce), bt_piece_get_size(pce));
    }

    close(fd);
}
#endif


/**
 * Used for initiation of downloading
 */
void bt_client_go(void *bto)
{
    bt_client_t *bt = bto;
    int ii;

    bt->net.peer_listen_open(&bt->net_udata,
            atoi(config_get(bt->cfg,"pwp_listen_port")));

    while (1)
    {
        if (0 == bt_client_step(bt))
            break;
    }

    __log(bt, NULL, "download is done\n");

    //__dumppiece(bt);
}

