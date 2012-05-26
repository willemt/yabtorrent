
/**
 * @file
 * @brief Manage connection with tracker
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


typedef struct
{
    /* how often we must send messages to the tracker */
    int interval;

    char *tracker_url;

    /* so that we remember when we last requested the peer list */
    time_t last_tracker_request;

    bt_client_cfg_t *cfg;

    bt_net_tracker_funcs_t net;

    void *net_udata;

    /** callback for initiating read of metafile */
    void (*func_read_metafile) (void *, char *, int len);
    void *udata;

} bt_tracker_client_t;

/*----------------------------------------------------------------------------*/

/**
 * Initiliase the tracker client
 *
 * @return tracker client on sucess; otherwise NULL
 *
 * \nosubgrouping
 */
void *bt_tracker_client_new(bt_client_cfg_t * cfg,
                            void (*metafile) (void *, char *, int len),
                            void *udata)
{
    bt_tracker_client_t *self;

    self = calloc(1, sizeof(bt_tracker_client_t));
    self->cfg = cfg;
    self->func_read_metafile = metafile;
    self->udata = udata;

    return self;
}

/**
 * Release all memory used by the tracker client
 *
 * @todo add destructors
 */
int bt_tracker_client_release(void *bto)
{
    //FIXME_STUB;

    return 1;
}

/*----------------------------------------------------------------------------*/

static void __build_tracker_request(bt_client_cfg_t * cfg, char **request)
{
    bt_piece_info_t *bi;
    char *info_hash_encoded;

    bi = &cfg->pinfo;
    info_hash_encoded = url_encode(cfg->info_hash);
    asprintf(request,
             "GET %s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%d&event=started&compact=1 http/1.0\r\n"
             "\r\n\r\n",
             cfg->tracker_url,
             info_hash_encoded,
             cfg->p_peer_id, cfg->pwp_listen_port, 0, 0,
             bi->npieces * bi->piece_len);
    free(info_hash_encoded);
}

static int __get_tracker_request(void *bto)
{
    bt_tracker_client_t *self = bto;
    int status = 0;
    char *host, *port, *default_port = "80";

//    printf("connecting to tracker: '%s:%s'\n", host, port);
    host = url2host(self->cfg->tracker_url);

    if (!(port = url2port(self->cfg->tracker_url)))
    {
        port = default_port;
    }

    if (1 ==
        self->net.tracker_connect(&self->net_udata, host, port,
                                  self->cfg->my_ip))
    {
        int rlen;
        char *request, *document, *response;

        __build_tracker_request(self->cfg, &request);

//        printf("my ip: %s\n", self->my_ip);
//        printf("requesting peer list: %s\n", request);
        if (
               /*  send http request */
               1 == self->net.tracker_send(&self->net_udata, request,
                                           strlen(request)) &&
               /*  receive http response */
               1 == self->net.tracker_recv(&self->net_udata, &response, &rlen))
        {
            int bencode_len;

            document = strstr(response, "\r\n\r\n") + 4;
            bencode_len = response + rlen - document;
            self->func_read_metafile(self->udata, document, bencode_len);
//            bt_tracker_client_read_tracker_response(self, document,
//                                                    bencode_len);
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

/**
 * Connect to tracker.
 *
 * Send request to tracker and get peer listing.
 *
 * @return 1 on sucess; otherwise 0
 *
 */
int bt_tracker_client_connect_to_tracker(void *bto)
{
    bt_tracker_client_t *bt = bto;

    assert(bt);
    assert(bt->cfg->tracker_url);
    assert(bt->cfg->info_hash);
    assert(bt->cfg->p_peer_id);
    return __get_tracker_request(bt);
}

void bt_tracker_client_step(void *bto, const long secs)
{
    bt_tracker_client_t *self = bto;

    /*  perform tracker request to get new peers */
    if (self->last_tracker_request + self->cfg->tracker_scrape_interval < secs)
    {
        self->last_tracker_request = secs;
        __get_tracker_request(self);
    }
}

/*----------------------------------------------------------------------------*/
/**
 * Set network functions
 */
void bt_tracker_client_set_net_funcs(void *bto, bt_net_tracker_funcs_t * net)
{
    bt_tracker_client_t *bt = bto;

    memcpy(&bt->net, net, sizeof(bt_net_tracker_funcs_t));
}
