
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

#include "bt.h"
#include "network_adapter.h"
#include "mock_torrent.h"

#include "bt_piece_db.h"
#include "bt_diskmem.h"
#include "config.h"
#include "linked_list_hashmap.h"
#include "bipbuffer.h"
#include "network_adapter_mock.h"

#include <fcntl.h>
#include <sys/time.h>

//void *__clients = NULL;

static unsigned long __vptr_hash(
    const void *e1
)
{
    return (unsigned long)e1;
}

static long __vptr_compare(
    const void *e1,
    const void *e2
)
{
    return (unsigned long)e1 - (unsigned long)e2;
}

#if 0
void __print_client_contents()
{
    hashmap_iterator_t iter;

    for (
        hashmap_iterator(__clients, &iter);
        hashmap_iterator_has_next(__clients, &iter);
        )
    {
        void* bt, *cfg;
        client_t* cli;
        hashmap_iterator_t iter_connects;

        cli = hashmap_iterator_next_value(__clients, &iter);

        /* loop throught each connection for this peer */
        for (
            hashmap_iterator(cli->connections, &iter_connects);
            hashmap_iterator_has_next(cli->connections, &iter_connects);
            )
        {
            client_connection_t* cn;
            int jj;

            cn = hashmap_iterator_next_value(cli->connections, &iter_connects);

#if 0 /* debugging */
            {
                void* data;
                printf("cli: %d nethandle: %d inbox: %dB ",
                        cli->nethandle, cn->nethandle, bipbuf_get_spaceused(cn->inbox));
                data = bipbuf_peek(cn->inbox);
                for (jj=0; jj<bipbuf_get_spaceused(cn->inbox); jj++)
                {
                    printf("%c", ((char*)data)[jj]);
                }
            }
            printf("\n");
#endif
        }
    }
}
#endif

/**
 * Create a connection to this peer */
static void __client_create_connection(client_t* me, void* nethandle)
{
    client_connection_t* cn;

    /* we already connected */
    if (hashmap_get(me->connections, nethandle))
    {
        return;
    }

    /* message inbox */
    cn = calloc(1,sizeof(client_connection_t));
    cn->inbox = bipbuf_new(1000);
    cn->connect_status = 0;
    cn->nethandle = nethandle;
    /* record on hashmap */
    hashmap_put(me->connections,cn->nethandle,cn);
}

/**
 * Put this message from nethandle onto my inbox for this nethandle
 * param nethandle: the nethandle that sent us this data */
static void __offer_inbox(client_t* me, const void* send_data, int len, void* nethandle)
{
    client_connection_t* cn;

    assert(me->connections);
    cn = hashmap_get(me->connections, nethandle);
    assert(cn);
    bipbuf_offer(cn->inbox, send_data, len);

#if 0 /*  debuggin */
    printf("|inbox me:%d rawpeer:%d peer:%d inbox:%d %dB",
            me->nethandle, nethandle, cn->nethandle, bipbuf_get_spaceused(cn->inbox), len);
    for (ii=0; ii<len; ii++)
        printf("%c", ((char*)send_data)[ii]);
    printf("\n");
    __print_client_contents();
#endif
}

void* networkfuns_mock_client_new(void* nethandle)
{
    client_t* cli;

    cli = calloc(1,sizeof(client_t));
    cli->connections = hashmap_new(__vptr_hash, __vptr_compare, 11);
    cli->nethandle = cli;
    return cli;
}

int peer_connect(
        void* caller,
        void **udata,
        void **nethandle,
        const char *host, const int port,
        int (*func_process_data) (void *caller,
                        void* nethandle,
                        const char* buf,
                        unsigned int len),
        int (*func_process_connection) (void *, void* nethandle, char *ip, int iplen),
        void (*func_connection_failed) (void *, void* nethandle)
        )
{
    client_t* you;
    client_t* me = *udata;

    /* nethandle is IP address */
    sscanf(host, "%p", nethandle);
    you = *nethandle;

#if 0 /*  debugging */
    printf("connecting me:%p nethandle:%p host:%s\n", me->nethandle, *nethandle, host);
#endif

    __client_create_connection(you, me);
    __client_create_connection(me, you);
    return 1;
}

/**
 *
 * @return 0 if added to buffer due to write failure, -2 if disconnect
 */
int peer_send(void* caller, void **udata,
              void* nethandle, const char *send_data, const int len)
{
    client_t* me = *udata;
    client_t* you;

#if 0 /* debugging */
    printf("send me:%d peer:%d len:%d\n", me->nethandle, nethandle, len);
#endif

    /* put onto the sendee's inbox */
    you = nethandle;
    __offer_inbox(you,send_data,len,me->nethandle);

    return 1;
}

int peer_disconnect(void* caller, void **udata, void* nethandle)
{
    return 1;
}

/**
 * poll info peer has information 
 * */
int network_poll(void* caller, void **udata,
               const int msec_timeout,
               int (*func_process) (void *caller,
                                    void* nethandle,
                                    const char* buf,
                                    unsigned int len),
               void (*func_process_connection) (void *,
                                                void* nethandle,
                                                char *ip,
                                                int port)
               )
{
    client_t* me = *udata;
    hashmap_iterator_t iter;

    /* loop throught each connection for this peer */
    for (
        hashmap_iterator(me->connections, &iter);
        hashmap_iterator_has_next(me->connections, &iter);
        )
    {
        client_connection_t* cn;

        cn = hashmap_iterator_next_value(me->connections, &iter);

        /* we need to process a connection request created by the peer */
        if (0 == cn->connect_status)
        {
            char ip[32];

            sprintf(ip, "%p", cn->nethandle);

#if 0 /* debugging */
            printf("processing connection me:%d them:%d\n",
                    me->nethandle, cn->nethandle);
#endif

            func_process_connection(me->bt, cn->nethandle, ip, 4000);
            cn->connect_status = CS_CONNECTED;
        }
        else if (!bipbuf_is_empty(cn->inbox))
        {
            int len = bipbuf_get_spaceused(cn->inbox);
            if (0 < len)
                func_process(me->bt,
                             (char*)cn->nethandle,
                             (char*)bipbuf_poll(cn->inbox, len), len);
        }
    }

    return 1;
}

