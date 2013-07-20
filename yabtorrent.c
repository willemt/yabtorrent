
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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>

#include <getopt.h>

#include "block.h"
#include "bt.h"
#include "bt_piece_db.h"
#include "bt_diskcache.h"
#include "bt_filedumper.h"
#include "bt_tracker_client.h"
#include "torrentfile_reader.h"
#include "readfile.h"
#include "config.h"
#include "networkfuncs.h"
#include "linked_list_queue.h"

#define PROGRAM_NAME "bt"

#if WIN32
#define _WIN32_WINNT 0x0501

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#include <uv.h>

#include <sys/time.h>

uv_loop_t *loop;


static void __log(void *udata, void *src, char *buf)
{
    char stamp[32];
    int fd = (unsigned long) udata;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    sprintf(stamp, "%d,%0.2f,", (int) tv.tv_sec, (float) tv.tv_usec / 100000);
    write(fd, stamp, strlen(stamp));
    write(fd, buf, strlen(buf));
}

static void __usage(int status)
{
#if 0
    if (status != EXIT_SUCCESS)
        fprintf(stderr, ("Try `%s --help' for more information.\n"),
                PROGRAM_NAME);
    else
#endif
    {
        printf("\
Usage: %s [OPTION]... TORRENT_FILE\n", PROGRAM_NAME);
        fputs(("\
Download torrent indicated by TORRENT_FILE. \n\n\
Mandatory arguments to long options are mandatory for short options too. \n\
  -i, --using-interface        network interface to use \n\
  -e, --verify-download        check downloaded files and quit \n\
  -t, --torrent_file_report_only    only report the contents of the torrent file \n\
  -b                                                    \n "), stdout);
        exit(status);
    }
}

static struct option const long_opts[] = {
    { "archive", no_argument, NULL, 'a'},
//  {" backup ", optional_argument, NULL, 'b'},
    /* The bounded network interface for net communications */
    { "using-interface", required_argument, NULL, 'i'},
    { "verify-download", no_argument, NULL, 'e'},
    { "shutdown-when-complete", no_argument, NULL, 's'},
    { "pwp_listen_port", required_argument, NULL, 'p'},
    { "torrent_file_report_only", required_argument, NULL, 't'},
    { NULL, 0, NULL, 0}
};

typedef struct {
    void* bt;

    /* piece db*/
    void* db;

    /* file dumper */
    void* fd;

    void* cfg;

    /* queue of announces to try */
    void* announces;

    char fname[256];
    int fname_len;
    int flen;

} torrent_reader_t;

int cb_event(void* udata, const char* key)
{
    return 1;
}

int cb_event_str(void* udata, const char* key, const char* val, int len)
{
    torrent_reader_t* me = udata;

#if 0 /* debugging */
    printf("%s %.*s\n", key, len, val);
#endif

    if (!strcmp(key,"announce"))
    {
        config_set_va(me->cfg,"tracker_url","%.*s", len, val);
        llqueue_offer(me->announces, strndup(val,len));
    }
    else if (!strcmp(key,"infohash"))
    {
        char* ihash;

        ihash = str2sha1hash(val, len);
        config_set_va(me->cfg,"infohash","%.*s", 20, ihash);
//        printf("hash: %.*s\n", 20, config_get(me->cfg,"infohash"));
    }
    else if (!strcmp(key,"pieces"))
    {
        int i;

        for (i=0; i < len; i += 20)
        {
            bt_piecedb_add(me->db,val + i);
        }

        config_set_va(me->cfg, "npieces", "%d",
                bt_piecedb_get_length(me->cfg));
        config_set_va(me->cfg, "npieces", "%d",
                bt_piecedb_get_length(me->cfg));
    }
    else if (!strcmp(key,"file path"))
    {
        assert(len < 256);
        strncpy(me->fname,val,len);
        me->fname_len = len;

//        config_set_va(me->cfg, config_get_int(
        bt_piecedb_increase_piece_space(me->db, me->flen);
        bt_filedumper_add_file(me->fd, me->fname, me->fname_len, me->flen);
    }

    return 1;
}

int cb_event_int(void* udata, const char* key, int val)
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
        config_set_va(me->cfg, "piece_length", "%d", val);
        printf("piece size: %d\n", val);
    }

    return 1;
}

/*******
 * FUNCTION
 *  Read metainfo file (ie. "torrent" file).
 *  This function will populate the piece database.
 * PARAMETERS
 *  bt bittorrent client
 *  db piece database 
 *  fd filedumper
 * RETURNS
 *  1 on sucess; otherwise 0
 ******/
static int __read_torrent_file(void* bt, void* db, void* fd, void* announces const char* torrent_file)
{
    void* tf;
    int len;
    char* metainfo;

    printf("\nReading file torrent file\n");
    memset(&r, 0, sizeof(torrent_reader_t));
    r.bt = bt;
    r.db = db;
    r.fd = fd;
    r.cfg = bt_client_get_config(bt);
    tf = tfr_new(cb_event, cb_event_str, cb_event_int, &r);
    metainfo = read_file(torrent_file,&len);
    tfr_read_metainfo(tf, metainfo, len);
    return 1;
}

static void __periodic(uv_timer_t* handle, int status)
{
    {
        void* bt;

        //bt_trackerclient_funcs_t *funcs
        
        bt =  bt_trackerclient_new(NULL);
        trackerclient_set_cfg(bt,bt_client_get_config(handle->data));
        bt_trackerclient_connect_to_uri(bt, "http://bt.ocremix.org/announce");
//        bt_trackerclient_connect_to_uri(bt, "http://bt.ocremix.org/announce");
//        bt_trackerclient_connect_to_uri(bt, "http://tracker.publicbt.com:80/announce");
//        bt_trackerclient_connect_to_uri(bt, "http://exodus.desync.com:80/announce");
//        bt_trackerclient_connect_to_uri(bt, "http://tracker.openbittorrent.com:80/announce");
//        bt_trackerclient_connect_to_uri(bt, "http://www.google.com:80/announce");
    }

}

int main(int argc, char **argv)
{
    char c;
    int o_verify_download, o_shutdown_when_complete, o_torrent_file_report_only;
    void *bt;
    char *str;
    config_t* cfg;
    torrent_reader_t reader;

    o_verify_download = 0;
    o_shutdown_when_complete = 0;
    o_torrent_file_report_only = 0;

    bt = bt_client_new();
    cfg = bt_client_get_config(bt);
    r.announces = llqueue_new();

#if 0
    status = config_read(cfg, "yabtc", "config");
    setlocale(LC_ALL, " ");
    atexit (close_stdin);
#endif

    while ((c = getopt_long(argc, argv, "esi:p:", long_opts, NULL)) != -1)
    {
        switch (c)
        {
        case 's':
            o_shutdown_when_complete = 1;
            break;
        case 'p':
            config_set_va(cfg,"pwp_listen_port","%.*s",strlen(optarg), optarg);
            break;
        case 't':
            config_set_va(cfg,"torrent_file","%.*s",strlen(optarg), optarg);
            o_torrent_file_report_only = 1;
            break;
        case 'i':
            config_set_va(cfg,"bounded_iface","%.*s",strlen(optarg), optarg);
            break;
        case 'e':
            o_verify_download = 1;
            break;

        default:
            __usage(EXIT_FAILURE);
        }
    }

    /* do configuration */
    config_set_va(cfg,"shutdown_when_complete","%d",o_shutdown_when_complete);
    config_set(cfg,"my_peerid",bt_generate_peer_id());
    assert(config_get(cfg,"my_peerid"));

    bt_client_set_logging(bt,
                          open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666),
                          __log);

    /* set network functions */
    {
        bt_client_funcs_t func = {
            .peer_connect = peer_connect,
            .peer_send =  peer_send,
            .peer_disconnect =peer_disconnect, 
            .peers_poll =peers_poll, 
            .peer_listen_open =peer_listen_open
        };

#if 0
        network_setup();
#endif

        bt_client_set_funcs(bt, &func, NULL);
    }

    /* set file system backend functions */
    void* fd, *dc, *db;

    bt_piecedb_i pdb_funcs = {
        .poll_best_from_bitfield = bt_piecedb_poll_best_from_bitfield,
        .get_piece = bt_piecedb_get
    };

    /* database for dumping pieces to disk */
    fd = bt_filedumper_new();

    /* intermediary between filedumper and DB */
    dc = bt_diskcache_new();
    //bt_diskcache_set_func_log(bt->dc, __log, bt);

    /* point diskcache to filedumper */
    bt_diskcache_set_disk_blockrw(dc, bt_filedumper_get_blockrw(fd), fd);

    /* set up piecedb */
    db = bt_piecedb_new();
    bt_piecedb_set_diskstorage(db,
            bt_diskcache_get_blockrw(dc),
            NULL,//(void*)bt_filedumper_add_file,
            dc);
    bt_client_set_piecedb(bt,&pdb_funcs,db);

    if (o_torrent_file_report_only)
    {
        __read_torrent_file(bt,db,fd,config_get(cfg,"torrent_file"));
        printf("\n");
    }

    if (argc == optind)
    {
        printf("ERROR: Please specify torrent file\n");
        exit(EXIT_FAILURE);
    }
    else if (0 == __read_torrent_file(&reader,argv[optind]))
    {
        exit(EXIT_FAILURE);
    }

    config_print(cfg);

    /* start uv */
    loop = uv_default_loop();
    /* create periodic timer */
    uv_timer_t *periodic_req;
    periodic_req = malloc(sizeof(uv_timer_t));
    periodic_req->data = bt;
    uv_timer_init(loop, periodic_req);
    uv_timer_start(periodic_req, __periodic, 0, 5000);
    uv_run(loop, UV_RUN_DEFAULT);

    if (o_verify_download)
    {

    }
    else
    {
        while (1)
        {
            if (0 == bt_client_step(bt))
                break;
        }

        __log(bt, NULL, "download is done\n");

    }

    bt_client_release(bt);
//    bt_connect_to_tracker(bt, bt_get_nbytes_downloaded)

    return uv_run(loop, UV_RUN_DEFAULT);
}
