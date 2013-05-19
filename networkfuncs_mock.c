
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

#include <fcntl.h>
#include <sys/time.h>

/**
 * Each client has many connections.
 * Each connection has an inbox */
typedef struct {
    void* inbox;

    /* id that we use to identify the peer */
    int peerid;

    /* 0 = not connected from our side, we need to accept the connection
     * 1 = connected */
    int connect_status;
} client_connection_t;

/* 
 *
 */
typedef struct {
    /* there is a connection for each peer */
    void* connections;

    /* the bitorrent client that represents this client */
    void* bt;

    /* id that we use to identify ourselves client.
     * This proxies our IP address */
    int peerid;
} client_t;

void *__clients = NULL;

/**
 * Create a connection to this peer */
static void __client_create_connection(client_t* cli, int peerid)
{
    client_connection_t* cn;

    /* we already connected */
    if (hashmap_get(cli->connections,&peerid))
    {
        return;
    }

    cn = calloc(0,sizeof(client_connection_t));

    /* message inbox */
    cn->inbox = cbuf_new(16);
    cn->peerid = peerid;
    cn->connect_status = 0;

    /* record on hashmap */
    hashmap_put(cli->connections,&cn->peerid,cn);
}

/**
 * Put this message from peerid onto my inbox for this peerid
 * param peerid: the peerid that sent us this data */
static void __offer_inbox(client_t* me, const void* send_data, int len, int peerid)
{
    client_connection_t* cn;

    cn = hashmap_get(me->connections, &peerid);

    assert(cn);

    cbuf_offer(cn->inbox, send_data, len);
}

client_t* __get_client_from_id(int peerid)
{
    client_t* cli;

    cli = hashmap_get(__clients, &peerid);

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



static void __log(void *udata, void *src, char *buf)
{
    char stamp[32];
    int fd = (unsigned long) udata;
    struct timeval tv;

    printf(buf);
    printf("\n");

    gettimeofday(&tv, NULL);
    sprintf(stamp, "%d,%0.2f,", (int) tv.tv_sec, (float) tv.tv_usec / 100000);
    write(fd, stamp, strlen(stamp));
    write(fd, buf, strlen(buf));
    write(fd, "\n", 1);
}

client_t* client_setup(int log)
{
    client_t* cli;
    void *bt;
    config_t* cfg;

    cli = calloc(0,sizeof(client_t));
    cli->connections = hashmap_new(__int_hash, __int_compare, 11);
    /* bittorrent client */
    cli->bt = bt = bt_client_new();
    cfg = bt_client_get_config(bt);
    //bt_client_set_peer_id(bt, bt_generate_peer_id());
    //bt_peerconn_set_my_peer_id

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

        bt_client_set_funcs(bt, &func, cli);
        bt_client_set_logging(bt,log , __log);
    }

    /* put inside the hashmap */
    cli->peerid = hashmap_count(__clients);
    hashmap_put(__clients,&cli->peerid,cli);
    printf("created client: %d\n", cli->peerid);

    return cli;
}

void* network_setup()
{
    int log;
    client_t* a, *b;
    hashmap_iterator_t iter;

    __clients = hashmap_new(__int_hash, __int_compare, 11);

    log = open("dump_log", O_CREAT | O_TRUNC | O_RDWR, 0666);

    a = client_setup(log);
    b = client_setup(log);

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
        config_set(cfg, "my_peerid", bt_generate_peer_id());
        bt_client_add_pieces(bt, "00000000000000000000", 1);
//        bt_client_add_pieces(bt, "00000000000000000000", 1);
        //bt_client_set_peer_id(bt, "00000000000000000000");
    }
#endif

    //bt_client_add_peer(a->bt,NULL,0,"1",1,0);
    bt_client_add_peer(b->bt,NULL,0,"0",1,0);

    int ii;

    for (ii=0; ii<1000; ii++)
    {
//        printf("\nStep %d:\n", ii+1);
        bt_client_step(a->bt);
        bt_client_step(b->bt);
    }

    return NULL;
}

/*----------------------------------------------------------------------------*/

int peer_connect(void **udata, const char *host, const char *port, int *peerid)
{
    client_t* you;
    client_t* me = *udata;

    /* peerid is IP address */
    *peerid = atoi(host);

    printf("connecting me:%d peerid:%d host:%s\n", me->peerid, *peerid, host);

    you = __get_client_from_id(*peerid);
    __client_create_connection(you, me->peerid);
    __client_create_connection(me, you->peerid);
    return 1;
}

/**
 *
 * @return 0 if added to buffer due to write failure, -2 if disconnect
 */
int peer_send(void **udata,
              const int peerid, const unsigned char *send_data, const int len)
{
    client_t* me = *udata;
    client_t* you;

    //printf("send me:%d peer:%d len:%d\n", me->peerid, peerid, len);

    /* put onto the sendee's inbox */
    you = __get_client_from_id(peerid);
    __offer_inbox(you,send_data,len,me->peerid);

    return 1;
}

/**
 * @param len: pointer to how much memory we need to read
 * @return how many we have read */
int peer_recv_len(void **udata, const int peerid, char *buf, int *len)
{
    client_t* me = *udata;
    client_connection_t* cn;
    void* data;

    cn = hashmap_get(me->connections, &peerid);

    assert(cn);
    assert(cn->inbox);

    //printf("recv me:%d peer:%d inbox:%d %dB\n",
    //        me->peerid, cn->peerid, cbuf_get_spaceused(cn->inbox), *len);


    /* no data on pipe */
#if 0
    if (cbuf_is_empty(cn->inbox))
    {
        return 0;
    }
#endif

    data = cbuf_poll(cn->inbox, (unsigned int)*len);

    /* we can't poll enough data */
    if (!data)
        return 0;

    /* put memory into buffer */
    memcpy(buf, data, *len);

    return 1;
}

int peer_disconnect(void **udata, int peerid)
{
    client_t* me = *udata;
    printf("disconnected\n");
    return 1;
}

/**
 * poll info peer has information 
 * */
int peers_poll(void **udata,
               const int msec_timeout,
               int (*func_process_msg) (void *a,
                                    int b),
               void (*func_process_connection) (void *,
                                                int netid,
                                                char *ip, int),
               void *caller)
{
    client_t* me = *udata;
    hashmap_iterator_t iter;

    hashmap_iterator(me->connections, &iter);
    while (hashmap_iterator_has_next(me->connections, &iter))
    {
        client_connection_t* cn;

        cn = hashmap_iterator_next_value(me->connections, &iter);

        /* we need to process a connection request created by the peer */
        if (0 == cn->connect_status)
        {
            char ip[32];

            sprintf(ip,"%d", cn->peerid);

            //printf("processing connection me:%d\n", me->peerid);
            func_process_connection(me->bt, cn->peerid, ip, 1);
            cn->connect_status = 1;
        }
        else if (!cbuf_is_empty(cn->inbox))
        {
            func_process_msg(me->bt, cn->peerid);
        }
    }

    return 1;
}

/**
 * open up to listen to peers */
int peer_listen_open(void **udata, const int port)
{
    client_t* me;

    printf("listen open\n");
    return 1;
}

/*----------------------------------------------------------------------------*/

