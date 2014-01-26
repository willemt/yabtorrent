
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/time.h>
#include <uv.h>

/* for INT_MAX */
#include <limits.h>

#include "bt.h"
#include "bt_piece_db.h"
#include "bt_diskcache.h"
#include "bt_filedumper.h"
#include "bt_string.h"
#include "bt_sha1.h"
#include "bt_selector_random.h"
#include "tracker_client.h"
#include "torrentfile_reader.h"
#include "readfile.h"
#include "config.h"
#include "networkfuncs.h"
#include "linked_list_queue.h"

#include "docopt.c"

#define PROGRAM_NAME "bt"

typedef struct {
    /* bitorrent client */
    void* bc;

    /* piece db*/
    void* db;

    /* file dumper */
    void* fd;

    /* disk cache */
    void* dc;

    /* configuration */
    void* cfg;

    /* queue of announces to try */
    void* announces;

    /* tracker client */
    void* tc;

    bt_dm_stats_t stat;

    uv_mutex_t mutex;
} sys_t;

typedef struct {
    sys_t* sys;
    char fname[256];
    int fname_len;
    int flen;
} torrent_reader_t;

uv_loop_t *loop;

static void __on_tc_done(void* data, int status);
static void __on_tc_add_peer(void* callee,
        char* peer_id,
        unsigned int peer_id_len,
        char* ip,
        unsigned int ip_len,
        unsigned int port);

static void __log(void *udata, void *src, const char *buf, ...)
{
    char stamp[32];
//    int fd = (unsigned long) udata;
    struct timeval tv;

    //printf("%s\n", buf);
    gettimeofday(&tv, NULL);
    sprintf(stamp, "%d,%0.2f,", (int) tv.tv_sec, (float) tv.tv_usec / 100000);
//    write(fd, stamp, strlen(stamp));
//    write(fd, buf, strlen(buf));
}

/**
 * Try to connect to this list of announces
 * @return 0 when no attempts could be made */
static int __trackerclient_try_announces(sys_t* me)
{
    void* tc;
    char* a; /* announcement */
    int waiting_for_response_from_connection = 0;

    if (0 == llqueue_count(me->announces))
        return 0;

    me->tc = tc = trackerclient_new(__on_tc_done, __on_tc_add_peer, me);
    trackerclient_set_cfg(tc, me->cfg);

    while ((a = llqueue_poll(me->announces)))
    {
        if (1 == trackerclient_supports_uri(tc, a))
        {
            printf("Trying: %s\n", a);

            if (0 == trackerclient_connect_to_uri(tc, a))
            {
                printf("ERROR: connecting to %s\n", a);
                goto skip;
            }
            waiting_for_response_from_connection = 1;
            free(a);
            break;
        }
        else
        {
            printf("ERROR: No support for URI: %s\n", a);
        }
skip:
        free(a);
    }

    if (0 == waiting_for_response_from_connection)
    {
        return 0;
    }

    return 1;
}

/**
 * Tracker client is done. */
static void __on_tc_done(void* data, int status)
{
    sys_t* me = data;

    if (0 == __trackerclient_try_announces(me))
    {
        printf("No connections made, quitting\n");
        exit(0);
    }
}

static void* on_call_exclusively(void* me, void* cb_ctx, void **lock, void* udata,
        void* (*cb)(void* me, void* udata))
{
    void* result;

    if (NULL == *lock)
    {
        *lock = malloc(sizeof(uv_mutex_t));
        uv_mutex_init(*lock);
    }

    uv_mutex_lock(*lock);
    result = cb(me,udata);
    uv_mutex_unlock(*lock);
    return result;
}

static int __dispatch_from_buffer(
        void *callee,
        void *peer_nethandle,
        const unsigned char* buf,
        unsigned int len)
{
    sys_t* me = callee;

    uv_mutex_lock(&me->mutex);
    bt_dm_dispatch_from_buffer(me->bc,peer_nethandle,buf,len);
    uv_mutex_unlock(&me->mutex);
    return 1;
}

static int __on_peer_connect(
        void *callee,
        void* peer_nethandle,
        char *ip,
        const int port)
{
    sys_t* me = callee;

    uv_mutex_lock(&me->mutex);
    bt_dm_peer_connect(me->bc,peer_nethandle,ip,port);
    uv_mutex_unlock(&me->mutex);

    return 1;
}

static void __on_peer_connect_fail(
    void *callee,
    void* peer_nethandle)
{
    sys_t* me = callee;

    uv_mutex_lock(&me->mutex);
    bt_dm_peer_connect_fail(me->bc,peer_nethandle);
    uv_mutex_unlock(&me->mutex);
}

/**
 * Tracker client wants to add peer. */
static void __on_tc_add_peer(void* callee,
        char* peer_id,
        unsigned int peer_id_len,
        char* ip,
        unsigned int ip_len,
        unsigned int port)
{
    sys_t* me = callee;
    void* peer;
    void* netdata;
    void* peer_nethandle;
    char ip_string[32];

    peer_nethandle = NULL;
    sprintf(ip_string,"%.*s", ip_len, ip);

#if 0 /* debug */
    printf("adding peer: %s %d\n", ip_string, port);
#endif

    uv_mutex_lock(&me->mutex);
    if (0 == peer_connect(me, &netdata, &peer_nethandle, ip_string, port,
                __dispatch_from_buffer,
                __on_peer_connect,
                __on_peer_connect_fail))
    {

    }
    peer = bt_dm_add_peer(me->bc, peer_id, peer_id_len, ip, ip_len, port, peer_nethandle);
    uv_mutex_unlock(&me->mutex);
}

static int __tfr_event(void* udata, const char* key)
{
    return 1;
}

static int __tfr_event_str(void* udata, const char* key, const char* val, int len)
{
    torrent_reader_t* me = udata;

#if 0 /* debugging */
    printf("%s %.*s\n", key, len, val);
#endif

    if (!strcmp(key,"announce"))
    {
        /* add to queue. We'll try them out by polling the queue elsewhere */
        llqueue_offer(me->sys->announces, strndup(val,len));
    }
    else if (!strcmp(key,"infohash"))
    {
        char hash[21];

        bt_str2sha1hash(hash, val, len);
        config_set_va(me->sys->cfg,"infohash","%.*s", 20, hash);
    }
    else if (!strcmp(key,"pieces"))
    {
        int i, bytes_used = 0, piece_size;

        piece_size = atoi(config_get(me->sys->cfg, "piece_length"));

        printf("piece size: %d\n", piece_size);

        /* do N-1 pieces */
        for (i=0; i < len - 20; i += 20)
        {
            bt_piecedb_add(me->sys->db, val + i, piece_size);
            bytes_used += piece_size;
        }

#if 1
        /* last piece probably has a different size... */
        int tot_size = bt_filedumper_get_total_size(me->sys->fd);
        assert(bytes_used < tot_size);
        assert(tot_size - bytes_used <= piece_size);
        bt_piecedb_add(me->sys->db, val + i, tot_size - bytes_used);
#endif

        config_set_va(me->sys->cfg, "npieces", "%d",
                bt_piecedb_get_length(me->sys->db));
    }
    else if (!strcmp(key,"file path"))
    {
        assert(len < 256);
        strncpy(me->fname,val,len);
        me->fname_len = len;
        bt_piecedb_increase_piece_space(me->sys->db, me->flen);
        bt_filedumper_add_file(me->sys->fd, me->fname, me->fname_len, me->flen);
    }

    return 1;
}

static int __tfr_event_int(void* udata, const char* key, int val)
{
    torrent_reader_t* me = udata;

#if 0 /* debugging */
    printf("%s %d\n", key, val);
#endif

    if (!strcmp(key,"file length"))
    {
        me->flen = val;
    }
    else if (!strcmp(key,"piece length"))
    {
        config_set_va(me->sys->cfg, "piece_length", "%d", val);
        //bt_piecedb_set_piece_length(me->sys->db, val);
        bt_diskcache_set_piece_length(me->sys->dc, val);
        bt_filedumper_set_piece_length(me->sys->fd, val);
    }

    return 1;
}

/**
 *  Read metainfo file (ie. "torrent" file).
 *  This function will populate the piece database.
 *  @param bc bittorrent client
 *  @param db piece database 
 *  @param fd filedumper
 *  @return 1 on sucess; otherwise 0
 */
static int __read_torrent_file(sys_t* me, const char* torrent_file)
{
    void* tf;
    int len;
    char* metainfo;
    torrent_reader_t r;

    memset(&r, 0, sizeof(torrent_reader_t));
    r.sys = me;
    tf = tfr_new(__tfr_event, __tfr_event_str, __tfr_event_int, &r);
    metainfo = read_file(torrent_file,&len);
    tfr_read_metainfo(tf, metainfo, len);
    return 1;
}

static void __log_process_info()
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

static int __cmp_peer_stats(const void * a, const void *b)
{
    return ((bt_dm_peer_stats_t*)b)->drate - ((bt_dm_peer_stats_t*)a)->drate;
}

static void __periodic(uv_timer_t* handle, int status)
{
    sys_t* me = handle->data;
    int i;

    if (me->bc)
    {
        uv_mutex_lock(&me->mutex);
        bt_dm_periodic(me->bc, &me->stat);
        uv_mutex_unlock(&me->mutex);
    }

    __log_process_info();

    // bt_piecedb_print_pieces_downloaded(bt_dm_get_piecedb(me));

    // TODO: show inactive peers
    // TODO: show number of invalid pieces

    int connected = 0, choked = 0, choking = 0, failed_connection = 0;
    int drate = 0, urate = 0, downloading = 0;
    int min=INT_MAX, max=0, avg=0;

    qsort(me->stat.peers, me->stat.npeers, sizeof(bt_dm_peer_stats_t),
        __cmp_peer_stats);

    for (i=0; i < me->stat.npeers; i++)
    {
        bt_dm_peer_stats_t* ps = &me->stat.peers[i];
        failed_connection += ps->failed_connection;
        connected += ps->connected;
        choking += ps->choking;
        choked += ps->choked;
        drate += ps->drate;
        urate += ps->urate;
        if (0 < ps->drate)
        {
            downloading += 1;
            avg += ps->drate;
            if (ps->drate < min) min = ps->drate;
            if (max < ps->drate) max = ps->drate;
        }
        if (0 == ps->drate || 5 < i) continue;
#if 0
        printf("peer (choked:%d choking:%d) "
                "dl:%04dKB/s ul:%04dKB/s\n",
                ps->choked, ps->choking,
                ps->drate == 0 ?  0 : ps->drate / 1000,
                ps->urate == 0 ?  0 : ps->urate / 1000
                );
#endif
    }

    if (0 < downloading)
        avg /= downloading;

    printf("%d/%d dl:%dKBs ",
        bt_piecedb_get_num_completed(me->db),
        bt_piecedb_get_length(me->db),
        drate == 0 ? 0 : drate / 1000);
    if (0 < drate)
        printf("(avg:%d min:%d max:%d) ",
                avg == 0 ? 0 : avg / 1000,
                min == 0 ? 0 : min / 1000,
                max == 0 ? 0 : max / 1000);
    printf("ul:%dKBs ", urate == 0 ? 0 : urate / 1000);
    printf("peer:%d actv:%d dlng:%d chkd:%d chkg:%d fail:%d ",
        me->stat.npeers, connected, downloading, choked, choking,
        failed_connection);
    printf("\t\t\t\t\r");
}

int main(int argc, char **argv)
{
    DocoptArgs args = docopt(argc, argv, 1, "0.1");
    sys_t me;

    me.bc = bt_dm_new();
    me.cfg = bt_dm_get_config(me.bc);
    me.announces = llqueue_new();
    memset(&me.stat, 0, sizeof(bt_dm_stats_t));

#if 0
    status = config_read(cfg, "yabtc", "config");
    setlocale(LC_ALL, " ");
    atexit (close_stdin);
    bt_dm_set_logging(bc,
            open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666), __log);
#endif

    if (args.torrent_file)
        config_set_va(me.cfg,"torrent_file","%s", args.torrent_file);
    config_set(me.cfg, "my_peerid", bt_generate_peer_id());
    assert(config_get(me.cfg, "my_peerid"));
    //if (o_show_config)
    //    config_print(cfg);

    /* database for dumping pieces to disk */
    me.fd = bt_filedumper_new();

    /* Disk Cache */
    me.dc = bt_diskcache_new();
    /* point diskcache to filedumper */
    bt_diskcache_set_disk_blockrw(me.dc,
            bt_filedumper_get_blockrw(me.fd), me.fd);

    /* Piece DB */
    me.db = bt_piecedb_new();
    bt_piecedb_set_diskstorage(me.db,
            bt_diskcache_get_blockrw(me.dc), me.dc);
    bt_dm_set_piece_db(me.bc,
            &((bt_piecedb_i) {
            .get_piece = bt_piecedb_get
            }), me.db);

    /* Selector */
    bt_dm_set_piece_selector(me.bc, &((bt_pieceselector_i) {
                .new = bt_random_selector_new,
                .peer_giveback_piece = bt_random_selector_giveback_piece,
                .have_piece = bt_random_selector_have_piece,
                .remove_peer = bt_random_selector_remove_peer,
                .add_peer = bt_random_selector_add_peer,
                .peer_have_piece = bt_random_selector_peer_have_piece,
                .get_npeers = bt_random_selector_get_npeers,
                .get_npieces = bt_random_selector_get_npieces,
                .poll_piece = bt_random_selector_poll_best_piece }), NULL);
    bt_dm_check_pieces(me.bc);
    bt_piecedb_print_pieces_downloaded(me.db);

    /* set network functions */
    bt_dm_set_cbs(me.bc, &((bt_dm_cbs_t) {
            .peer_connect = peer_connect,
            .peer_send = peer_send,
            .peer_disconnect = peer_disconnect, 
            .call_exclusively = on_call_exclusively,
            .log = __log
            }), NULL);

    if (args.info)
        __read_torrent_file(&me, config_get(me.cfg,"torrent_file"));

    if (argc == optind)
    {
        printf("%s", args.help_message);
        exit(EXIT_FAILURE);
    }
    else if (0 == __read_torrent_file(&me,argv[optind]))
    {
        exit(EXIT_FAILURE);
    }

    /* start uv */
    loop = uv_default_loop();
    uv_mutex_init(&me.mutex);

    /* create periodic timer */
    uv_timer_t *periodic_req;
    periodic_req = malloc(sizeof(uv_timer_t));
    periodic_req->data = &me;
    uv_timer_init(loop, periodic_req);
    uv_timer_start(periodic_req, __periodic, 0, 1000);

    /* open listening port */
    void* netdata;
    int listen_port = args.port ? atoi(args.port) : 0;
    if (0 == (listen_port = peer_listen(&me, &netdata, listen_port,
                __dispatch_from_buffer,
                __on_peer_connect,
                __on_peer_connect_fail)))
    {
        printf("ERROR: can't create listening socket");
        exit(0);
    }

    config_set_va(me.cfg, "pwp_listen_port", "%d", listen_port);
    printf("Listening on port: %d\n", listen_port);

    /* try to connect to tracker */
    if (0 == __trackerclient_try_announces(&me))
    {
        printf("No connections made, quitting\n");
        exit(0);
    }

    uv_run(loop, UV_RUN_DEFAULT);
    bt_dm_release(me.bc);
    return 1;
}
