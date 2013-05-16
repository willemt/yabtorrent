
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

#include "block.h"
#include "bt.h"
#include "networkfuncs.h"

/*----------------------------------------------------------------------------*/

#include "bt_diskmem.h"
#include "config.h"
#include "linked_list_hashmap.h"
#include "cbuffer.h"

typedef struct {
    void* inbox;
    void* bt;

    /* id that we use to identify the client */
    int id;
} client_t;

void *__clients = NULL;

client_t* __get_client_from_id(int id)
{
    client_t* cli;

    cli = hashmap_get(__clients, &id);

    return cli;
}

static unsigned long __int_hash(
    const void *e1
)
{
    const int *i1 = e1;

    return *i1;
}

static long __int_compare(
    const void *e1,
    const void *e2
)
{
    const int *i1 = e1, *i2 = e2;

    return *i1 - *i2;
}

client_t* client_setup()
{
    client_t* cli;
    void *bt;
    config_t* cfg;

    cli = malloc(sizeof(client_t));

    /* message inbox */
    cli->inbox = cbuf_new(16);

    /* bittorrent client */
    cli->bt = bt = bt_client_new();
    cfg = bt_client_get_config(bt);
    //bt_client_set_peer_id(bt, bt_generate_peer_id());

    /* create disk backend */
    {
        void* dc;

        dc = bt_diskmem_new();
        bt_diskmem_set_size(dc, 1000);
        bt_client_set_diskstorage(bt, bt_diskmem_get_blockrw(dc), NULL, dc);
    }

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

        bt_client_set_funcs(bt, &func, NULL);
    }

    /* put inside the hashmap */
    cli->id = hashmap_count(__clients);
    hashmap_put(__clients,&cli->id,cli);

    return cli;
}

void* network_setup()
{
    __clients = hashmap_new(__int_hash, __int_compare, 11);

    client_t* a, *b;

    hashmap_iterator_t iter;

    a = client_setup();
    b = client_setup();

#if 1
    hashmap_iterator(__clients, &iter);
    while (hashmap_iterator_has_next(__clients, &iter))
    {
        void* bt, *cfg;
        client_t* cli;

        cli = hashmap_iterator_next_value(__clients, &iter);
        bt = cli->bt;
        cfg = bt_client_get_config(bt);
        config_set(cfg, "npieces", "1");
        config_set(cfg, "piece_length", "5");
        config_set(cfg, "infohash", "00000000000000000000");
        config_set(cfg, "my_peerid", "00000000000000000000");
        bt_client_add_pieces(bt, "00000000000000000000", 1);
        bt_client_add_pieces(bt, "00000000000000000000", 1);
        //bt_client_set_peer_id(bt, "00000000000000000000");
    }
#endif

    bt_client_add_peer(a->bt,NULL,0,"0",0,0);

    bt_client_step(a->bt);

    bt_client_step(b->bt);

    return NULL;
}

/*----------------------------------------------------------------------------*/

int peer_connect(void **udata, const char *host, const char *port, int *peerid)
{
    printf("connecting\n");
//    *peerid = 1;
    return 1;
}

/**
 *
 * @return 0 if added to buffer due to write failure, -2 if disconnect
 */
int peer_send(void **udata,
              const int peerid, const unsigned char *send_data, const int len)
{
    client_t* me;

    printf("sending: peerid:%d len:%d\n", peerid, len);

    me = __get_client_from_id(peerid);
    //cbuf_offer(me->inbox, send_data, len);
    return 1;
}

/**
 * @return how many we have read */
int peer_recv_len(void **udata, const int peerid, char *buf, int *len)
{
    client_t* me;
    void* data;

    me = __get_client_from_id(peerid);

    printf("receiving: inbox:%d peer:%d %dB\n", cbuf_get_spaceused(me->inbox), peerid, *len);

    data = cbuf_poll(me->inbox, (unsigned int)len);

    /* we can't poll enough data */
    if (!data)
        return 0;

    memcpy(buf,  data, *len);
    //cbuf_poll_release(me->inbox, *len);

    return 1;
}

int peer_disconnect(void **udata, int peerid)
{
    printf("disconnected\n");
    return 1;
}

/**
 * poll info peer has information 
 * */
int peers_poll(void **udata,
               const int msec_timeout,
               int (*func_process) (void *a,
                                    int b),
               void (*func_process_connection) (void *,
                                                int netid,
                                                char *ip, int), void *data)
{
    printf("polling\n");
    return 1;
}

/**
 * open up to listen to peers */
int peer_listen_open(void **udata, const int port)
{
    printf("listen open\n");
    return 1;
}

/*----------------------------------------------------------------------------*/

