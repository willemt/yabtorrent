
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
#include <uv.h>

#include "block.h"
#include "bt.h"

typedef struct {
    int (*func_process_data) (void *caller,
                        void* nethandle,
                        const unsigned char* buf,
                        unsigned int len);
    int (*func_process_connection) (void *, void* nethandle, char *ip, int iplen);
    void (*func_process_connection_fail) (void *, void* nethandle);
    void* callee;
    /*  socket for sending on */
    uv_stream_t* stream;
    //uv_tcp_t* socket;
} connection_attempt_t;

static void __alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    buf->len = size;
    buf->base = malloc(size);
}

static void __read_cb(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
    connection_attempt_t *ca = tcp->data;

    if (nread >= 0)
    {
        ca->func_process_data(ca->callee, ca, buf->base, nread);
    }
    else
    {

    }

    free(buf->base);
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
//        fprintf(stderr, "connect callback error %s\n",
//                uv_err_name(uv_last_error(uv_default_loop())));
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
        int (*func_process_connection) (void *, void* nethandle, char *ip, int iplen),
        void (*func_connection_failed) (void *, void* nethandle))
{
    uv_connect_t *c;
    uv_tcp_t *t;
    struct sockaddr addr;
    connection_attempt_t *ca;

    *nethandle = ca = calloc(1,sizeof(connection_attempt_t));
    ca->func_process_data = func_process_data;
    ca->func_process_connection = func_process_connection;
    ca->func_process_connection_fail = func_connection_failed;
    ca->callee = caller;

#if 0 /* debugging */
    printf("connecting to: %lx %s:%d\n", ca, host, port);
#endif
    
    t = malloc(sizeof(uv_tcp_t));
    if (0 != uv_tcp_init(uv_default_loop(), t))
    {
        printf("FAILED TCP socket creation\n");
        return 0;
    }

    uv_ip4_addr(host, port, (struct sockaddr_in*)&addr);

    c = malloc(sizeof(uv_connect_t));
    c->data = ca;
    if (0 != uv_tcp_connect(c, t, &addr, __on_connect))
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

