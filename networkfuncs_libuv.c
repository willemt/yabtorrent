
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
    int (*func_process_data) (void *caller,
                        void* nethandle,
                        const unsigned char* buf,
                        unsigned int len);
    void (*func_process_connection) (void *, void* nethandle, char *ip, int iplen);
    void (*func_process_connection_fail) (void *, void* nethandle);
    void* callee;
    /*  socket for sending on */
    uv_stream_t* stream;
    //uv_tcp_t* socket;
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
//        printf("read: %d\n", nread);
        ca->func_process_data(ca->callee, ca, buf.base, nread);
    }
    else
    {
//        printf("done reading: %d\n", nread);
    }

    free(buf.base);
}

static void __write_cb(uv_write_t *req, int status)
{
//    free(req);
}

static void __on_connect(uv_connect_t *req, int status)
{
    connection_attempt_t *ca = req->data;
    char *request;
    int r;

    ca->stream = req->handle;

    assert(req->data);

    if (status == -1)
    {
        fprintf(stderr, "connect callback error %s\n",
                uv_err_name(uv_last_error(uv_default_loop())));
        ca->func_process_connection_fail(ca->callee,ca);
        return;
    }

    /*  start reading from peer */
    req->handle->data = req->data;
    r = uv_read_start(req->handle, __alloc_cb, __read_cb);

    ca->func_process_connection(ca->callee, ca, "", 0);
}

int peer_connect(void* caller,
        void **udata,
        void **nethandle,
        const char *host, int port,
        int (*func_process_data) (void *caller,
                        void* nethandle,
                        const unsigned char* buf,
                        unsigned int len),
        void (*func_process_connection) (void *, void* nethandle, char *ip, int iplen),
        void (*func_connection_failed) (void *, void* nethandle))
{
    uv_connect_t *connect_req;
    uv_tcp_t *socket;
    struct sockaddr_in addr;
    connection_attempt_t *ca;

    *nethandle = ca = calloc(1,sizeof(connection_attempt_t));
    ca->func_process_data = func_process_data;
    ca->func_process_connection = func_process_connection;
    ca->func_process_connection_fail = func_connection_failed;
    ca->callee = caller;

#if 1 /* debugging */
    printf("connecting to: %lx %s:%d\n", ca, host, port);
#endif
    
    addr = uv_ip4_addr(host, port);
    connect_req = malloc(sizeof(uv_connect_t));
    connect_req->data = ca;
    socket = malloc(sizeof(uv_tcp_t));

    if (0 != uv_tcp_init(uv_default_loop(), socket))
    {
        printf("FAILED TCP socket creation\n");
        return 0;
    }

    if (0 != uv_tcp_connect(connect_req, socket, addr, __on_connect))
    {
        printf("FAILED connection creation\n");
        return 0;
    }

    return 1;
}

/**
 *
 * @return 0 if added to buffer due to write failure, -2 if disconnect
 */
int peer_send(void* caller, void **udata, void* nethandle,
        const unsigned char *send_data, const int len)
{
    connection_attempt_t *ca;
    uv_write_t *req;
    uv_buf_t buf;
    int r;

    ca = nethandle;

    if (!ca->stream)
    {
        fprintf(stderr, "unable to send as not connected\n");
        return 0;
    }

    /*  create buffer */
    //buf = uv_buf_init((char*) malloc(len), len);
    //memcpy(buf.base, send_data, len);
    buf.base = (void*)send_data;
    buf.len = len;

    /*  write */
    req = malloc(sizeof(uv_write_t));
    r = uv_write(req, ca->stream, &buf, 1, __write_cb);

    return 1;
}

int peer_disconnect(void* caller, void **udata, void* nethandle)
{
    return 1;
}

