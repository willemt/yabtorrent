
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
#include "bt_main.h"
#include "config.h"

#define PROGRAM_NAME "bt"

/**
 * The bounded network interface for net communications */
char __cfg_bound_iface[32];

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
  -b                                                    \n "), stdout);
        exit(status);
    }
}

static struct option const long_opts[] = {
    {
     "archive", no_argument, NULL, 'a'},
//  {" backup ", optional_argument, NULL, 'b'},
    {
     "using-interface", required_argument, NULL, 'i'},
    {"verify-download", no_argument, NULL, 'e'},
    {"shutdown-when-complete", no_argument, NULL, 's'},
    {"pwp_listen_port", required_argument, NULL, 'p'},
    {
     NULL, 0, NULL, 0}
};

int main(int argc, char **argv)
{
    char c;
    int o_verify_download, o_shutdown_when_complete;
    void *bt;

    o_verify_download = 0;
    o_shutdown_when_complete = 0;

    bt = bt_client_new();

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
            printf("opt: %s\n", optarg);
            bt_client_set_opt(bt, "pwp_listen_port", optarg, strlen(optarg));
            break;

        case 'i':
            strcpy(__cfg_bound_iface, optarg);
            printf("interface: %s\n", optarg);
            break;

        case 'e':
            o_verify_download = 1;
            break;

        default:
            __usage(EXIT_FAILURE);
        }
    }

    char *str;
    int status;

    /* do configuration */

    config_t* cfg;

    status = config_read(cfg, " yabtc ", " config ");

#if 0
    if ((str = config_get(" max_peer_connections ")))
        bt_client_set_max_peer_connections(bt, atoi(str));
    if ((str = config_get(" select_timeout_msec ")))
        bt_client_set_select_timeout_msec(bt, atoi(str));
    if ((str = config_get(" max_cache_mem_mb ")))
        bt_client_set_max_cache_mem(bt, atoi(str));
#endif

    bt_client_set_config(bt, cfg);

    bt_client_set_logging(bt,
                          open("dump_log", O_CREAT | O_TRUNC | O_RDWR,
                               0666), __log);

    bt_client_set_opt_shutdown_when_completed(bt, o_shutdown_when_complete);

    bt_client_set_path(bt, ".");
    bt_client_set_peer_id(bt, bt_generate_peer_id());

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
