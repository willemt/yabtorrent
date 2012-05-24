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

#include <string.h>
#include <stdarg.h>

#include <arpa/inet.h>

#include "bt.h"
#include "bt_local.h"
#include "url_encoder.h"

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
 *
 *
 */

typedef struct
{
    /*  the size of the piece, etc */
    bt_piece_info_t pinfo;

    /* 20-byte self-designated ID of the peer */
    char *p_peer_id;

    /* name of the topmost directory */
    char *name;

    /* the main directory of the file */
    char *path;

    /* how often we must send messages to the tracker */
    int interval;

    char *tracker_url;
    void **peerconnects;
    int npeers;

    /* sha1 hash of the info_hash */
    char *info_hash;

    /*  the piece container */
    bt_piecedb_t *db;

    /*  disk cache */
    void *dc;

    /*  file dumper */
    bt_filedumper_t *fd;

    /* net stuff */

    bt_net_funcs_t net;

    void *net_udata;


    /* number of complete peers */
    int ncomplete_peers;

    char fail_reason[255];

    /* so that we remember when we last requested the peer list */
    time_t last_tracker_request;

    /*  are we seeding? */
    int am_seeding;

    /*------------------------------------------------*/

    /*  don't seed, shutdown when complete */
    int o_shutdown_when_complete;
    /*  how many seconds between tracker scrapes */
//    int o_tracker_scrape_interval;

    /*------------------------------------------------*/

    /*  logging  */

//    bt_logger_t logger;
    func_log_f func_log;
    void *log_udata;

    /*  this holds my IP. I figure it out */
    char my_ip[32];
//    char my_port[8];

    /* listen for pwp messages on this port */
    int pwp_listen_port;

    bt_client_cfg_t cfg;
    bt_pwp_cfg_t pwpcfg;

    /*  leeching choker */
    void *lchoke;

    void *ticker;
} bt_client_t;

/*----------------------------------------------------------------------------*/

/**
 * print a string of all the downloaded pieces */
static void __print_pieces_downloaded(void *bto)
{
    bt_client_t *bt = bto;

    int ii, counter, is_complete = 1;

    printf("pieces downloaded: ");

    for (ii = 0, counter = 20;
         ii < bt_piecedb_get_length(bt->db); ii++, counter += 1)
    {
        bt_piece_t *pce;

        pce = bt_piecedb_get(bt->db, ii);

        if (bt_piece_is_complete(pce))
        {
            printf("1");
        }
        else
        {
            printf("0");
            is_complete = 0;
        }

        if (counter == 80)
        {
            counter = 2;
            printf("\n  ");
        }
    }
    printf("\n");

    if (is_complete)
    {

    }
}

/**
 * @return 1 if all complete, 0 otherwise */
static int __all_pieces_are_complete(bt_client_t * bt)
{
    int ii;

    for (ii = 0; ii < bt_piecedb_get_length(bt->db); ii++)
    {
        bt_piece_t *pce = bt_piecedb_get(bt->db, ii);

        if (!bt_piece_is_complete(pce))
        {
            return 0;
        }
    }

    return 1;
}

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

/*
 * peer connections are given this as a callback whenever they want to send
 * information */
static int __FUNC_peerconn_send_to_peer(void *bto,
                                        bt_peer_t * peer,
                                        void *data, const int len)
{
    bt_client_t *bt = bto;

    return bt->net.peer_send(&bt->net_udata, peer->net_peerid, data, len);
}

static int __FUNC_peercon_pollblock(void *bto,
                                    bt_bitfield_t * peer_bitfield,
                                    bt_block_t * blk)
{
    bt_client_t *bt = bto;

//    bt_block_tpwp_piece_block_t req;
    bt_piece_t *pce;

    pce = bt_piecedb_poll_best_from_bitfield(bt->db, peer_bitfield);
    if (pce)
    {
        //    bt_piece_build_request(pce, blk);
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

/*  return how many we've read */
static int __FUNC_peercon_recv(void *bto, bt_peer_t * peer, char *buf, int *len)
{
    bt_client_t *bt = bto;

//        bt->net.peer_recv(&bt->net_udata, peer->net_peerid, &msg, &len);
    return bt->net.peer_recv_len(&bt->net_udata, peer->net_peerid, buf, len);
}

/**
 * Received a block from a peer
 *
 * @param peer : peer received from
 * */
static int __FUNC_peercon_pushblock(void *bto,
                                    bt_peer_t * peer,
                                    bt_block_t * block, void *data)
{
    bt_client_t *bt = bto;
    bt_piece_t *pce;

    pce = bt_client_get_piece(bt, block->piece_idx);
    assert(pce);
    bt_piece_write_block(pce, NULL, block, data);
//    bt_filedumper_write_block(bt->fd, block, data);

    if (bt_piece_is_complete(pce))
    {
//        printf("COMPLETED %d\n", bt_piece_is_valid(pce));
        /*  send have to all peers */
        if (bt_piece_is_valid(pce))
        {
            int ii;

            __log(bt, NULL,
                  "client,piece downloaded,pieceidx=%d\n",
                  bt_piece_get_idx(pce));
            for (ii = 0; ii < bt->npeers; ii++)
            {
                void *pc;

                pc = bt->peerconnects[ii];
                if (!bt_peerconn_is_active(pc))
                    continue;
                bt_peerconn_send_have(pc, bt_piece_get_idx(pce));
            }
        }

        __print_pieces_downloaded(bt);

        if (__all_pieces_are_complete(bt))
        {
            bt->am_seeding = 1;
            bt_diskcache_disk_dump(bt->dc);
        }
    }

    return 1;
}

/*----------------------------------------------------------------------------*/

static int __have_peer(void *bto, const char *ip, const int port)
{
    bt_client_t *bt = bto;
    int ii;

    for (ii = 0; ii < bt->npeers; ii++)
    {
        bt_peer_t *peer;

        peer = bt_peerconn_get_peer(bt->peerconnects[ii]);
        if (!strcmp(peer->ip, ip) && atoi(peer->port) == port)
        {
            return 1;
        }
    }
    return 0;
}

/*
'announce':
    This is a string value. It contains the announce URL of the tracker. 
'announce-list':
    This is an OPTIONAL list of string values. Each value is a URL pointing to a backup tracker. This value is not used in BTP/1.0. 
'comment':
    This is an OPTIONAL string value that may contain any comment by the author of the torrent. 
'created by':
    This is an optional string value and may contain the name and version of the program used to create the metainfo file. 
'creation date':
    This is an OPTIONAL string value. It contains the creation time of the torrent in standard Unix epoch format. 
'info':
    This key points to a dictionary that contains information about the files to download. The entries are explained in the following sections. 

4.1.1 Single File Torrents

If the torrent only specifies one file, the info dictionary must have the following keys:

'length':
    This is an integer value indicating the length of the file in bytes. 
'md5sum':
    This is an OPTIONAL value. If included it must be a string of 32 characters corresponding to the MD5 sum of the file. This value is not used in BTP/1.0. 
'name':
    A string containing the name of the file. 
'piece length':
    An integer indicating the number of bytes in each piece. 
'pieces':
    This is a string value containing the concatenation of the 20-byte SHA1 hash value for all pieces in the torrent. For example, the first 20 bytes of the string represent the SHA1 value used to verify piece index 0. 

4.1.2 Multi File Torrents

If the torrent specifies multiple files, the info dictionary must have the following structure:

'files':
    This is a list of dictionaries. Each file in the torrent has a dictionary associated to it having the following structure:

        'length':
            This is an integer indicating the total length of the file in bytes. 
        'md5sum':
            This is an OPTIONAL value. if included it must be a string of 32 characters corresponding to the MD5 sum of the file. This value is not used in BTP/1.0. 
        'path':
            This is a list of string elements that specify the path of the file, relative to the topmost directory. The last element in the list is the name of the file, and the elements preceding it indicate the directory hierarchy in which this file is situated. 

'name':
    This is a string value. It contains the name of the top-most directory in the file structure. 
'piece length':
    This is an integer value. It contains the number of bytes in each piece. 
'pieces':
    This is a string value. It must contain the concatenation of all 20-byte SHA1 hash values that are used by BTP/1.0 to verify each downloaded piece. The first 20 bytes of the string represent the SHA1 value used to verify piece index 0. 
*/

/*----------------------------------------------------------------------------*/

static void *__netpeerid_to_peerconn(bt_client_t * bt, const int netpeerid)
{
    int ii;

    for (ii = 0; ii < bt->npeers; ii++)
    {
        void *pc = bt->peerconnects[ii];
        bt_peer_t *peer;

//        if (!bt_peerconn_is_active(pc))
//            continue;
        peer = bt_peerconn_get_peer(pc);
        if (peer->net_peerid == netpeerid)
            return pc;
    }

    assert(FALSE);
    return NULL;
}

/**
 * Take this message and process it on the PeerConnection side
 * */
static int __process_peer_msg(void *bto, const int netpeerid)
{
    bt_client_t *bt = bto;
    void *pc;

    pc = __netpeerid_to_peerconn(bt, netpeerid);
    bt_peerconn_process_msg(pc);

    return 1;
}

static void __process_peer_connect(void *bto,
                                   const int netpeerid,
                                   char *ip, const int port)
{
    bt_client_t *bt = bto;
    bt_peer_t *peer;

//    void *pc;

    peer = bt_client_add_peer(bt, NULL, 0, ip, strlen(ip), port);
    peer->net_peerid = netpeerid;
//    pc = __netpeerid_to_peerconn(bt, netpeerid);
}

#include <sys/time.h>
#include <sys/resource.h>

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
    struct rusage stats;
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

    getrusage(RUSAGE_SELF, &stats);
    __log(bt, NULL,
          "stats, ru_utime=%d, ru_stime=%d, ru_maxrss=%d, ru_ixrss=%d, ru_idrss=%d, ru_isrss=%d, ru_minflt=%d, ru_majflt=%d, ru_nswap=%d, ru_inblock=%d, ru_oublock=%d, ru_msgsnd=%d, ru_msgrcv=%d, ru_nsignals=%d, ru_nvcsw=%d, ru_nivcsw=%d",
          stats.ru_utime, stats.ru_stime, stats.ru_maxrss,
          stats.ru_ixrss, stats.ru_idrss, stats.ru_isrss,
          stats.ru_minflt, stats.ru_majflt,
          stats.ru_nswap, stats.ru_inblock,
          stats.ru_oublock, stats.ru_msgsnd,
          stats.ru_msgrcv, stats.ru_nsignals, stats.ru_nvcsw, stats.ru_nivcsw);
}

/**
 * Run peerconnection step.
 * Ensure we are connected to our assigned peers.
 * */
static void __peerconn_step(bt_client_t * bt, int ii)
{
    void *pc;

//    bt_peer_t *peer;

    pc = bt->peerconnects[ii];
//    peer = bt_peerconn_get_peer(pc);

    bt_peerconn_step(pc);
}

/*----------------------------------------------------------------------------*/
static void __FUNC_peerconn_log(void *bto, void *src_peer, const char *buf, ...)
{
    bt_client_t *bt = bto;

    bt_peer_t *peer = src_peer;

    char buffer[256];

    sprintf(buffer, "pwp,%s,%s\n", peer->peer_id, buf);
    bt->func_log(bt->log_udata, NULL, buffer);
}

static int __FUNC_peerconn_pieceiscomplete(void *bto, void *piece)
{
    bt_client_t *bt = bto;

    bt_piece_t *pce = piece;

    return bt_piece_is_complete(pce);
}

/**
 *
 * @return info hash */
static char *__FUNC_peerconn_get_infohash(void *bto, bt_peer_t * peer)
{
    bt_client_t *bt = bto;

    return bt->info_hash;
}

static int __FUNC_peerconn_disconnect(void *udata,
                                      bt_peer_t * peer, char *reason)
{
//    printf("DISCONNECTING: %s\n", reason);
    return 1;
}

static int __FUNC_peerconn_connect(void *bto, void *pc, bt_peer_t * peer)
{
    bt_client_t *bt = bto;

    /* the remote peer will have always send a handshake */
    if (!bt->net.peer_connect)
    {
        return 0;
    }

    if (0 == bt->net.peer_connect(&bt->net_udata, peer->ip,
                                  peer->port, &peer->net_peerid))
    {
        printf("failed connection to peer\n");
        return 0;
    }

    return 1;
}

static int __get_drate(const void *bto, const void *pc)
{
    const bt_client_t *bt = bto;

    return bt_peerconn_get_download_rate(pc);
}

static int __get_urate(const void *bto, const void *pc)
{
    const bt_client_t *bt = bto;

    return bt_peerconn_get_upload_rate(pc);
}

static int __get_is_interested(void *bto, void *pc)
{
    bt_client_t *bt = bto;

    return bt_peerconn_is_interested(pc);
}

static void __choke_peer(void *bto, void *pc)
{

    bt_client_t *bt = bto;

    bt_peerconn_choke(pc);
//    pc->isChoked = 1;
}

static void __unchoke_peer(void *bto, void *pc)
{
    bt_client_t *bt = bto;

//    pr->isChoked = 0;
    bt_peerconn_unchoke(pc);
//    pc->isChoked = 1;
}

static bt_choker_peer_i iface_choker_peer = {
    .get_drate = __get_drate,.get_urate =
        __get_urate,.get_is_interested =
        __get_is_interested,.choke_peer =
        __choke_peer,.unchoke_peer = __unchoke_peer
};

static void __leecher_peer_reciprocation(void *bto)
{
    bt_client_t *bt = bto;

    bt_leeching_choker_decide_best_npeers(bt->lchoke);
    bt_ticker_push_event(bt->ticker, 10, bt, __leecher_peer_reciprocation);
}

static void __leecher_peer_optimistic_unchoke(void *bto)
{
    bt_client_t *bt = bto;

    bt_leeching_choker_optimistically_unchoke(bt->lchoke);
    bt_ticker_push_event(bt->ticker, 30, bt, __leecher_peer_optimistic_unchoke);
}

/*----------------------------------------------------------------------------*/
/** @name Setup
 */
/* @{ */

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

    bt->ticker = bt_ticker_new();
    bt->db = bt_piecedb_new();
    bt->fd = bt_filedumper_new();
    bt->dc = bt_diskcache_new();

    /* configuration */
    sprintf(bt->my_ip, "127.0.0.1");
    bt->pwp_listen_port = 6000;
    bt->cfg.max_peer_connections = 10;
    bt->cfg.select_timeout_msec = 1000;
    bt->cfg.tracker_scrape_interval = 10;
    bt->cfg.max_active_peers = 4;
    bt->pwpcfg.max_pending_requests = 10;

    /*  set leeching choker */
    bt->lchoke = bt_leeching_choker_new(bt->cfg.max_active_peers);
    bt_leeching_choker_set_choker_peer_iface(bt->lchoke, bt,
                                             &iface_choker_peer);

    /*  set diskcache logger and blockrw */
    bt_diskcache_set_disk_blockrw(bt->dc,
                                  bt_filedumper_get_blockrw(bt->fd), bt->fd);
    bt_diskcache_set_func_log(bt->dc, __log, bt);

    /* point piece database to diskcache */
    bt_piecedb_set_diskstorage(bt->db,
                               bt_diskcache_get_blockrw(bt->dc), bt->dc);

    /*  start reciprocation timer */
    bt_ticker_push_event(bt->ticker, 10, bt, __leecher_peer_reciprocation);

    /*  start optimistic unchoker timer */
    bt_ticker_push_event(bt->ticker, 30, bt, __leecher_peer_optimistic_unchoke);

    return bt;
}

/**
 * Release all memory used by the client
 *
 * Close all peer connections
 * @todo add destructors
 */
int bt_client_release(void *bto)
{
    //FIXME_STUB;

    return 1;
}

/*----------------------------------------------------------------------------*/
/** @name Content
 */
/* @{ */

/**
 * Read metainfo file (ie. "torrent" file)
 *
 * This function will populate the piece database
 *
 * @return 1 on sucess; otherwise 0
 */
int bt_client_read_metainfo_file(void *bto, const char *fname)
{
    bt_client_t *bt = bto;
    char *contents;
    int len;

    if (!(contents = read_file(fname, &len)))
    {
        printf("ERROR: torrent file: %s can't be read\n", fname);
        return 0;
    }

    bt_client_read_metainfo(bto, contents, len, &bt->pinfo);


    __print_pieces_downloaded(bto);

    if (__all_pieces_are_complete(bt))
    {
        bt->am_seeding = 1;
    }

    free(contents);

    return 1;
}

static void __build_tracker_request(bt_client_t * bt, char **request)
{
    bt_piece_info_t *bi;
    char *info_hash_encoded;

    bi = &bt->pinfo;
    info_hash_encoded = url_encode(bt->info_hash);
    asprintf(request,
             "GET %s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%d&event=started&compact=1 http/1.0\r\n"
             "\r\n\r\n",
             bt->tracker_url,
             info_hash_encoded,
             bt->p_peer_id, bt->pwp_listen_port, 0, 0,
             bi->npieces * bi->piece_len);
    free(info_hash_encoded);
}

static int __get_tracker_request(void *bto)
{
    bt_client_t *bt = bto;
    int status = 0;
    char *host, *port, *default_port = "80";

//    printf("connecting to tracker: '%s:%s'\n", host, port);
    host = url2host(bt->tracker_url);
    if (!(port = url2port(bt->tracker_url)))
    {
        port = default_port;
    }

    if (1 == bt->net.tracker_connect(&bt->net_udata, host, port, bt->my_ip))
    {
        int rlen;
        char *request, *document, *response;

        __build_tracker_request(bt, &request);
//        printf("my ip: %s\n", bt->my_ip);
//        printf("requesting peer list: %s\n", request);
        if (
               /*  send http request */
               1
               ==
               bt->net.tracker_send(&bt->net_udata, request, strlen(request)) &&
               /*  receive http response */
               1 == bt->net.tracker_recv(&bt->net_udata, &response, &rlen))
        {
            int bencode_len;

            document = strstr(response, "\r\n\r\n") + 4;
            bencode_len = response + rlen - document;
            bt_client_read_tracker_response(bt, document, bencode_len);
            free(request);
            free(response);
            status = 1;
        }
    }

    free(host);
    if (port != default_port)
        free(port);
    return status;
}

/* @} ------------------------------------------------------------------------*/
/** @name Tracker
 * @{
 */

/**
 * Connect to tracker.
 *
 * Send request to tracker and get peer listing.
 *
 * @return 1 on sucess; otherwise 0
 *
 */
int bt_client_connect_to_tracker(void *bto)
{
    bt_client_t *bt = bto;

    assert(bt);
    assert(bt->tracker_url);
    assert(bt->info_hash);
    assert(bt->p_peer_id);
    return __get_tracker_request(bt);
}

/* @} ------------------------------------------------------------------------*/
/** @name Piece Management
 * @{
 */

/**
 * Obtain this piece from the piece database
 *
 * @return piece specified by piece_idx; otherwise NULL
 */
bt_piece_t *bt_client_get_piece(void *bto, const int piece_idx)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get(bt->db, piece_idx);
}

/**
 * Add a piece to the piece database
 * @return 1 on sucess; otherwise 0
 * */
int bt_client_add_piece(void *bto, const char *sha1)
{
    bt_client_t *bt = bto;

    bt_piecedb_add(bt->db, sha1);
    bt->pinfo.npieces = bt_piecedb_get_length(bt->db);
    return 1;
}

/**
 * Mass add pieces to the piece database
 *
 * @param pieces A string of 20 byte sha1 hashes. This string is always a multiple of 20 bytes in length. 
 * @param bto the bittorrent client object
 * */
void bt_client_add_pieces(void *bto, const char *pieces, int len)
{
    int prog;

    prog = 0;
    while (prog <= len)
    {
        prog++;
        if (0 == prog % 20)
        {
            bt_client_add_piece(bto, pieces);
            pieces += 20;
        }
    }
}

/**
 * Add this file to the bittorrent client
 *
 * This is used for adding new files.
 *
 * @param id id of bt client
 * @param fname file name
 * @param fname_len length of fname
 * @param flen length in bytes of the file
 *
 * @return 1 on sucess; otherwise 0
 */
int bt_client_add_file(void *bto,
                       const char *fname, const int fname_len, const int flen)
{
    bt_client_t *bt = bto;
    char *path = NULL;

    printf("adding file: %.*s (%d bytes) flen:%d\n",
           fname_len, fname, flen, fname_len);
//    bt->tot_file_size += flen;
    bt_piecedb_set_tot_file_size(bt->db,
                                 bt_piecedb_get_tot_file_size(bt->db) + flen);
    asprintf(&path, "%.*s", fname_len, fname);
    bt_filedumper_add_file(bt->fd, path, flen);
    return 1;
}

/* @} ------------------------------------------------------------------------*/
/** @name Peer Management
 * @{
 */
/**
 * Add the peer.
 *
 * Initiate connection with 
 *
 * @return freshly created bt_peer
 */
bt_peer_t *bt_client_add_peer(void *bto,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port)
{
    bt_client_t *bt = bto;
    void *pc;
    bt_peer_t *peer;

    /*  peer is me.. */
    if (!strncmp(ip, bt->my_ip, ip_len) && port == bt->pwp_listen_port)
    {
        return NULL;
    }
    /* prevent dupes.. */
    else if (__have_peer(bt, ip, port))
    {
        return NULL;
    }

    peer = calloc(1, sizeof(bt_peer_t));
    /*  'compact=0'
     *  doesn't use peerids.. */
    if (peer_id)
    {
        asprintf(&peer->peer_id, "00000000000000000000");
    }
    else
    {
        asprintf(&peer->peer_id, "", peer_id_len, peer_id);
    }
    asprintf(&peer->ip, "%.*s", ip_len, ip);
    asprintf(&peer->port, "%d", port);
    pc = bt_peerconn_new();
    bt_peerconn_set_peer(pc, peer);
    bt_peerconn_set_func_getpiece(pc, bt_client_get_piece);
    bt_peerconn_set_func_push_block(pc, __FUNC_peercon_pushblock);
    bt_peerconn_set_func_send(pc, __FUNC_peerconn_send_to_peer);
    bt_peerconn_set_func_recv(pc, __FUNC_peercon_recv);
    bt_peerconn_set_func_pollblock(pc, __FUNC_peercon_pollblock);
    bt_peerconn_set_func_log(pc, __FUNC_peerconn_log);
    bt_peerconn_set_func_disconnect(pc, __FUNC_peerconn_disconnect);
    bt_peerconn_set_func_connect(pc, __FUNC_peerconn_connect);
    bt_peerconn_set_func_get_infohash(pc, __FUNC_peerconn_get_infohash);
    bt_peerconn_set_func_piece_is_complete(pc, __FUNC_peerconn_pieceiscomplete);
    bt_peerconn_set_pieceinfo(pc, &bt->pinfo);
    bt_peerconn_set_isr_udata(pc, bt);
    bt_peerconn_set_pwp_cfg(pc, &bt->pwpcfg);
    bt_peerconn_set_peer_id(pc, bt->p_peer_id);
    printf("adding peer: ip:%.*s port:%d\n", ip_len, ip, port);
    bt->npeers++;
    bt->peerconnects = realloc(bt->peerconnects, sizeof(void *) * bt->npeers);
    bt->peerconnects[bt->npeers - 1] = pc;
    bt_leeching_choker_add_peer(bt->lchoke, pc);
    return peer;
}

/**
 * Remove the peer.
 *
 * Disconnect the peer
 *
 * @todo add disconnection functionality
 *
 * @return 1 on sucess; otherwise 0
 *
 */
int bt_client_remove_peer(void *bto, const int peerid)
{
//    bt_client_t *bt = bto;
//    bt->peerconnects[bt->npeers - 1]

//    bt_leeching_choker_add_peer(bt->lchoke, peer);
    return 1;
}

/* @} ------------------------------------------------------------------------*/
/** @name Download Management
 * @{ */


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
    time_t seconds;

    __log_process_info(bt);
    seconds = time(NULL);

    /*  shutdown if we are setup to not seed */
    if (1 == bt->am_seeding && 1 == bt->o_shutdown_when_complete)
    {
        return 0;
    }

    /*  perform tracker request to get new peers */
    if (bt->last_tracker_request + bt->cfg.tracker_scrape_interval < seconds)
    {
        bt->last_tracker_request = seconds;
        __get_tracker_request(bt);
    }

    /*  poll data from peer pwp connections */
    bt->net.peers_poll(&bt->net_udata,
                       bt->cfg.select_timeout_msec,
                       __process_peer_msg, __process_peer_connect, bt);

    /*  run each peer connection step */
    for (ii = 0; ii < bt->npeers; ii++)
    {
        __peerconn_step(bt, ii);
    }

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

    bt->net.peer_listen_open(&bt->net_udata, bt->pwp_listen_port);

    while (1)
    {
        if (0 == bt_client_step(bt))
            break;
    }

    printf("download is done!\n");
    int fd;

    fd = open("piecedump", O_CREAT | O_WRONLY);
    for (ii = 0; ii < bt_piecedb_get_length(bt->db); ii++)
    {
        bt_piece_t *pce = bt_piecedb_get(bt->db, ii);

        write(fd, bt_piece_get_data(pce), bt_piece_get_size(pce));
    }

    close(fd);
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

/* @} ------------------------------------------------------------------------*/
/** @name Properties
 * @{ */

/**
 * Set the maximum number of peer connection
 */
void bt_client_set_max_peer_connections(void *bto, const int npeers)
{
    bt_client_t *bt = bto;

    bt->cfg.max_peer_connections = npeers;
}

/**
 * Set timeout in milliseconds
 */
void bt_client_set_select_timeout_msec(void *bto, const int val)
{
    bt_client_t *bt = bto;

    bt->cfg.select_timeout_msec = val;
}

/**
 * Set maximum amount of megabytes used by piece cache
 */
void bt_client_set_max_cache_mem(void *bto, const int val)
{
    bt_client_t *bt = bto;

    bt->cfg.max_cache_mem = val;
}

/**
 * Set the number of completed peers
 */
void bt_client_set_num_complete_peers(void *bto, const int npeers)
{
    bt_client_t *bt = bto;

    bt->ncomplete_peers = npeers;
}

/**
 * Set the failure flag to true
 */
void bt_client_set_failed(void *bto, const char *reason)
{

}

/**
 * Set tracker correspondence interval
 * @todo implement
 */
void bt_client_set_interval(void *bto, int interval)
{

}

/**
 * Set the client's peer id
 */
void bt_client_set_peer_id(void *bto, char *peer_id)
{
    bt_client_t *bt = bto;
    int ii;

    bt->p_peer_id = peer_id;
    for (ii = 0; ii < bt->npeers; ii++)
    {
        bt_peerconn_set_peer_id(bt->peerconnects[ii], peer_id);
    }
}

/**
 * Get the client's peer id
 */
char *bt_client_get_peer_id(void *bto)
{
    bt_client_t *bt = bto;

    return bt->p_peer_id;
}

/**
 * How many pieces are there of this file?
 *
 * <b>Note:</b> bt_read_torrent_file sets this
 * The size of a piece is determined by the publisher of the torrent. A good recommendation is to use a piece size so that the metainfo file does not exceed 70 kilobytes.
 */
void bt_client_set_npieces(void *bto, int npieces)
{
    assert(FALSE);
// fixed_piece_size = size_of_torrent / number_of_pieces
    bt_client_t *bt = bto;

    bt->pinfo.npieces = npieces;
}

/**
 * Set file download path
 * @see bt_client_read_metainfo_file
 */
void bt_client_set_path(void *bto, const char *path)
{
    bt_client_t *bt = bto;

    bt->path = strdup(path);
    bt_filedumper_set_path(bt->fd, bt->path);
}

/**
 * Set piece length
 * @see bt_client_read_metainfo_file
 */
void bt_client_set_piece_length(void *bto, const int len)
{
    bt_client_t *bt = bto;

    bt->pinfo.piece_len = len;
    bt_piecedb_set_piece_length(bt->db, len);
    bt_filedumper_set_piece_length(bt->fd, len);
    bt_diskcache_set_size(bt->dc, len);
}

/**
 * Set the logging function
 */
void bt_client_set_logging(void *bto, void *udata, func_log_f func)
{
    bt_client_t *bt = bto;

    bt->func_log = func;
    bt->log_udata = udata;
}

/**
 * Set network functions
 */
void bt_client_set_net_funcs(void *bto, bt_net_funcs_t * net)
{
    bt_client_t *bt = bto;

    memcpy(&bt->net, net, sizeof(bt_net_funcs_t));
}

/**
 * Set shutdown when completed flag
 *
 * If this is set, the client will shutdown when the download is completed.
 *
 * @return 1 on success, 255 if the option doesn't exist, 0 otherwise
 * */
void bt_client_set_opt_shutdown_when_completed(void *bto, const int flag)
{
    bt_client_t *bt = bto;

    bt->o_shutdown_when_complete = flag;
}

/**
 * Set a key-value option
 *
 * Options include:
 * pwp_listen_port - The port the client will listen on for PWP connections
 * tracker_interval - Length in seconds that client will communicate with the tracker
 * tracker_url - Set tracker's url
 * infohash - Set the client's infohash
 *
 * @param key The option
 * @param val The value
 * @param val_len Value's length
 *
 */
int bt_client_set_opt(void *bto,
                      const char *key, const char *val, const int val_len)
{
    bt_client_t *bt = bto;

    if (!strcmp(key, "pwp_listen_port"))
    {
        bt->pwp_listen_port = atoi(val);
        return 1;
    }
    else if (!strcmp(key, "tracker_interval"))
    {
        bt->interval = atoi(val);
        return 1;
    }
    else if (!strcmp(key, "tracker_url"))
    {
        bt->tracker_url = calloc(1, sizeof(char) * (val_len + 1));
        strncpy(bt->tracker_url, val, val_len);
        return 1;
    }
    else if (!strcmp(key, "infohash"))
    {
        int ii;

        bt->info_hash = strdup(val);
#if 0
        printf("Info hash.....: ");
        for (ii = 0; ii < 20; ii++)
            printf("%02x", val[ii]);
        printf("\n");
#endif

        return 1;
    }
    else if (!strcmp(key, "tracker_backup"))
    {

        return 1;
    }

    return 255;
}

/**
 * Get a key-value option
 *
 * @return string of value if applicable, otherwise NULL
 */
char *bt_client_get_opt_string(void *bto, const char *key)
{
    bt_client_t *bt = bto;

    if (!strcmp(key, "tracker_url"))
    {
        return bt->tracker_url;
    }
    else if (!strcmp(key, "infohash"))
    {
        return bt->info_hash;
    }
    else
    {
        return NULL;
    }
}

/**
 * Get a key-value option
 *
 * @return integer of value if applicable, otherwise -1
 */
int bt_client_get_opt_int(void *bto, const char *key)
{
    bt_client_t *bt = bto;

    if (!strcmp(key, "pwp_listen_port"))
    {
        return bt->pwp_listen_port;
    }
    else if (!strcmp(key, "tracker_interval"))
    {
        return bt->interval;
    }
    else
    {
        return -1;
    }
}


/**
 * @return number of peers this client is involved with
 */
int bt_client_get_num_peers(void *bto)
{
    bt_client_t *bt = bto;

    return bt->npeers;
}

/**
 * @return number of pieces in this torrent
 */
int bt_client_get_num_pieces(void *bto)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get_length(bt->db);
}

/**
 * @return the total size of the file as specified by the torrent
 * @see bt_client_read_metainfo_file
 */
int bt_client_get_total_file_size(void *bto)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get_tot_file_size(bt->db);
}

/**
 * @return reason for failure
 */
char *bt_client_get_fail_reason(void *bto)
{
    bt_client_t *bt = bto;

    return bt->fail_reason;
}

/**
 * @return interval that we have to communicate with the tracker
 */
int bt_client_get_interval(void *bto)
{
    bt_client_t *bt = bto;

    return bt->interval;
}

/* @} ------------------------------------------------------------------------*/
