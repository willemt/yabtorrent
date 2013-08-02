
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
#include <uv.h>

#include "block.h"
#include "bt.h"

typedef struct {
    void (*func_process_connection) (void *, void* nethandle, char *ip, int iplen);
    void* callee;
    void* nethandle;
} connection_attempt_t;

static uv_buf_t __alloc_cb(uv_handle_t* handle, size_t size)
{
  return uv_buf_init(malloc(size), size);
}

static void __read_cb(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf)
{
    connection_attempt_t *ca = tcp->data;

    if (nread >= 0)
    {

    }
    else
    {
        printf("done reading\n");
    }

    free(buf.base);
}

static void __on_connect(uv_connect_t *req, int status)
{
    connection_attempt_t *ca = req->data;
    int r;
    char *request;
    uv_buf_t buf;
    uv_write_t *write_req;

    assert(req->data);

    printf("connected.\n");

    if (status == -1)
    {
        fprintf(stderr, "connect callback error %s\n",
                uv_err_name(uv_last_error(uv_default_loop())));
        return;
    }

    req->handle->data = req->data;
//    write_req = malloc(sizeof(uv_write_t));
//    r = uv_write(write_req, req->handle, &buf, 1, __write_cb);
    r = uv_read_start(req->handle, __alloc_cb, __read_cb);

    ca->func_process_connection(ca->callee,ca->nethandle, NULL, 0);
}

int peer_connect(void **udata, const char *host, int port, void **nethandle,
        void (*func_process_connection) (void *, void* nethandle, char *ip, int iplen))
{
    uv_connect_t *connect_req;
    uv_tcp_t *socket;
    struct sockaddr_in addr;
    connection_attempt_t *ca;

    ca = malloc(sizeof(connection_attempt_t));

//    *peerid = 1;
//    printf("connecting to: %s:%d\n", host, port);
    
    addr = uv_ip4_addr(host, port);
    connect_req = malloc(sizeof(uv_connect_t));
    connect_req->data = ca;
    socket = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), socket);
    uv_tcp_connect(connect_req, socket, addr, __on_connect);

    return 1;
}

/**
 *
 * @return 0 if added to buffer due to write failure, -2 if disconnect
 */
int peer_send(void **udata,
              void* nethandle, const unsigned char *send_data, const int len)
{

    return 1;
}

int peer_disconnect(void **udata, void* nethandle)
{
    return 1;
}

#if 0
/*
 * poll info peer has information 
 * */
int peers_poll(void **udata,
               const int msec_timeout,
               int (*func_process) (void *caller,
                                    void* netid,
                                    const unsigned char* buf,
                                    unsigned int len),
               void (*func_process_connection) (void *,
                                                int netid,
                                               char *ip, int), void *data)

    return 1;
}

/*
 * open up to listen to peers */
int peer_listen_open(void **udata, const int port)
{
    return 1;
}

#endif
