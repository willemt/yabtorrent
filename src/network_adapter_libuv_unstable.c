
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

#include "bt.h"

static void __fatal(int status)
{
    //printf("ERROR: %s %s\n", uv_err_name(status), uv_strerror(status));
    assert(0);
}

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
} connection_attempt_t;

static uv_buf_t __alloc_cb(uv_handle_t* handle, size_t size)
{
    uv_buf_t buf;
    buf.len = size;
    buf.base = malloc(size);
    return buf;
}

static void __read_cb(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf)
{
    connection_attempt_t *ca = tcp->data;

    if (nread >= 0)
    {
        ca->func_process_data(ca->callee, ca, (const unsigned char*)buf.base, nread);
    }
    else
    {

    }

    free(buf.base);
}

static void __write_cb(uv_write_t *req, int status)
{
    if (0 != status)
        __fatal(status);
//    free(req);
}

static void __on_connect(uv_connect_t *req, int status)
{
    connection_attempt_t *ca = req->data;
    char *request;
    int r;

    ca->stream = req->handle;

#if 0 /* debugging */
    printf("connected: 0x%lx %d\n", (unsigned long)ca, status);
#endif

    assert(req->data);

//    if (0 != status)
//        __fatal(status);

    if (status != 0)
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
    printf("connecting to: %lx %s:%d 0x%lx\n",
            ca, host, port, (unsigned long)ca);
#endif
    
    t = malloc(sizeof(uv_tcp_t));
    if (0 != uv_tcp_init(uv_default_loop(), t))
    {
        printf("FAILED TCP socket creation\n");
        return 0;
    }

    addr = uv_ip4_addr(host, port);//, (struct sockaddr_in*)&addr);

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

static void __on_new_connection_from_listen(uv_stream_t *t, int status)
{
    /* TCP client socket */
    uv_tcp_t *tc;
    connection_attempt_t *ca, *ca_me;

    if (0 != status)
        __fatal(status);

#if 1 /* debug */
    printf("new connection from listen\n");
#endif

    tc = malloc(sizeof(uv_tcp_t));
    if (0 != uv_tcp_init(uv_default_loop(), tc))
    {
        printf("FAILED TCP socket creation\n");
        return;
    }

    ca_me = t->data;

    /* we need to create a new connection attempt for this peer */
    tc->data = ca = calloc(1,sizeof(connection_attempt_t));
    ca->func_process_data = ca_me->func_process_data;
    ca->func_process_connection = ca_me->func_process_connection;
    ca->func_process_connection_fail = ca_me->func_process_connection_fail;
    ca->callee = ca_me->callee;
    ca->stream = (uv_stream_t*)tc;

    if (uv_accept(t, (uv_stream_t*)tc) == 0)
    {
        uv_read_start((uv_stream_t*)tc, __alloc_cb, __read_cb);

        /* get peer's ip and port */
        int ip_len = 3 * 4 + 3 + 1;
        char* ip = malloc(ip_len);
        struct sockaddr_in peer_addr;
        int namelen = sizeof(peer_addr);
        if (0 != uv_tcp_getsockname((uv_tcp_t*)tc,
                    (struct sockaddr*)&peer_addr,
                    &namelen))
        {
        }

        uv_ip4_name(&peer_addr, ip, ip_len);

        ca->func_process_connection(ca->callee, ca,
                ip, ntohs(peer_addr.sin_port));
    }
    else
    {
        uv_close((uv_handle_t*) tc, NULL);
    }
}

/**
 * Open up a socket for accepting connections
 * @return port for listening on success, otherwise 0 */
int peer_listen(void* caller,
        void **nethandle,
        int port,
        int (*func_process_data) (void *caller,
                        void* nethandle,
                        const unsigned char* buf,
                        unsigned int len),
        int (*func_process_connection) (void *, void* nethandle, char *ip, int iplen),
        void (*func_connection_failed) (void *, void* nethandle))
{
    uv_tcp_t *t;
    connection_attempt_t *ca;

    *nethandle = ca = calloc(1,sizeof(connection_attempt_t));
    ca->func_process_data = func_process_data;
    ca->func_process_connection = func_process_connection;
    ca->func_process_connection_fail = func_connection_failed;
    ca->callee = caller;

    t = malloc(sizeof(uv_tcp_t));
    if (0 != uv_tcp_init(uv_default_loop(), t))
    {
        printf("FAILED TCP socket creation\n");
        return 0;
    }

    struct sockaddr_in bind_addr;
    
    uv_ip4_addr("0.0.0.0", port, &bind_addr);
    uv_tcp_bind(t, (const struct sockaddr*)&bind_addr, 0);
    t->data = ca;
    if (0 != uv_listen((uv_stream_t*)t, 128, __on_new_connection_from_listen))
    {
        printf("ERROR: listen error\n");
        return 0;
    }

    int namelen = sizeof(bind_addr);
    uv_tcp_getsockname(t, (struct sockaddr*)&bind_addr, &namelen);

    return ntohs(bind_addr.sin_port);
}

