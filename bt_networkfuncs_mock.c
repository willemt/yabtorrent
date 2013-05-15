
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
#include "bt_main.h"

#include "cbuffer.h"

/*----------------------------------------------------------------------------*/

#if 0
/*  @param my_ip: populate this with the interface's ip */
int tracker_connect(void **udata,
                    const char *host, const char *port, char *my_ip)
{
    return 1;
}

int tracker_send(void **udata, const void *data, int len)
{
    return 1;
}

int tracker_recv(void **udata, char **rdata, int *rlen)
{
    return 1;
}

int tracker_disconnect(void **udata)
{

    return 1;
}
#endif

/*----------------------------------------------------------------------------*/

int peer_connect(void **udata, const char *host, const char *port, int *peerid)
{
    *peerid = 1;
    return 1;
}

/**
 *
 * @return 0 if added to buffer due to write failure, -2 if disconnect
 */
int peer_send(void **udata,
              const int peerid, const unsigned char *send_data, const int len)
{

    return 1;
}

/*  return how many we've read */
int peer_recv_len(void **udata, const int peerid, char *buf, int *len)
{
    return 0;
}

int peer_disconnect(void **udata, int peerid)
{
    return 1;
}

/*
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
    return 1;
}

/*----------------------------------------------------------------------------*/

/*
 * open up to listen to peers */
int peer_listen_open(void **udata, const int port)
{
    return 1;
}

/*----------------------------------------------------------------------------*/

bt_net_pwp_funcs_t pwpNetFuncs = {
    peer_connect,
    peer_send,
    peer_recv_len,
    peer_disconnect,
    peers_poll,
    peer_listen_open
};

