
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
#include "bt_diskcache.h"
#include "bt_filedumper.h"
#include "bt_tracker_client.h"
#include "torrentfile_reader.h"
#include "readfile.h"
#include "config.h"
#include "networkfuncs.h"

#define PROGRAM_NAME "bt"


#include <sys/time.h>

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
    void* cfg;

} torrent_reader_t;

int cb_event(void* udata, const char* key)
{
    printf("%s\n");

    return 1;
}

int cb_event_str(void* udata, const char* key, const char* val, int len)
{
    torrent_reader_t* me = udata;

    printf("%s %.*s\n", key, len, val);

    if (!strcmp(key,"announce"))
    {
        config_set_va(me->cfg,"tracker_url","%.*s", len, val);
    }


    return 1;
}

int cb_event_int(void* udata, const char* key, int val)
{
    printf("%s %d\n", key, val);

    return 1;
}

void __read_torrent_file(void* bt, char* torrent_file)
{
    torrent_reader_t r;
    void* tf;
    int len;
    char* metainfo;

    memset(&r, 0, sizeof(torrent_reader_t));
    r.bt = bt;
    r.cfg = bt_client_get_config(bt);
    tf = tfr_new(cb_event, cb_event_str, cb_event_int, &r);
    metainfo = read_file(torrent_file,&len);
    tfr_read_metainfo(tf, metainfo, len);
}

int main(int argc, char **argv)
{
    char c;
    int o_verify_download, o_shutdown_when_complete, o_torrent_file_report_only;
    void *bt;
    char *str;
    //int status;
    config_t* cfg;

    o_verify_download = 0;
    o_shutdown_when_complete = 0;
    o_torrent_file_report_only = 0;

    bt = bt_client_new();
    cfg = bt_client_get_config(bt);
    //status = config_read(cfg, "yabtc", "config");

    // setlocale(LC_ALL, " ");
    //  atexit (close_stdin);

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

    /* set peer id */
    bt_client_set_peer_id(bt, bt_generate_peer_id());

    bt_client_set_logging(bt,
                          open("dump_log", O_CREAT | O_TRUNC | O_RDWR,
                               0666), __log);

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

        network_setup();
        bt_client_set_funcs(bt, &func, NULL);
    }

    /* set file system backend functions */
    {
        void* fd, *dc;

        /* database for dumping pieces to disk */
        fd = bt_filedumper_new();

        /* intermediary between filedumper and DB */
        dc = bt_diskcache_new();
        //bt_diskcache_set_func_log(bt->dc, __log, bt);

        /* point diskcache to filedumper */
        bt_diskcache_set_disk_blockrw(dc, bt_filedumper_get_blockrw(fd), fd);
        bt_client_set_diskstorage(bt,
                bt_diskcache_get_blockrw(dc),
                (void*)bt_filedumper_add_file, dc);
    }

    if (o_torrent_file_report_only)
    {
        __read_torrent_file(bt,config_get(cfg,"torrent_file"));
        printf("\n");
    }

    config_print(cfg);

    if (argc == optind)
    {
        printf("ERROR: Please specify torrent file\n");
        exit(EXIT_FAILURE);
    }
    else if (0 == bt_client_read_metainfo_file(bt, argv[optind]))
    {
        exit(EXIT_FAILURE);
    }

    if (o_verify_download)
    {

    }
    else
    {

        while (1)
        {
            //bt_trackerclient_step(tc);

            if (0 == bt_client_step(bt))
                break;
        }

        __log(bt, NULL, "download is done\n");

    }

    bt_client_release(bt);
//    bt_connect_to_tracker(bt, bt_get_nbytes_downloaded)

    return 1;
}
