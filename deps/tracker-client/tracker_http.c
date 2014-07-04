
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 

 * @file
 * @brief Manage connection with tracker
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include <assert.h>

/* for vargs */
#include <stdarg.h>

#include <uv.h>
#include <http_parser.h>

#include "config.h"

/* for uri encoding */
#include "uri.h"

#include "tracker_client.h"
#include "tracker_client_private.h"
#include "bencode.h"

#define HTTP_PREFIX "http://"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

typedef struct {
    trackerclient_t *tc;

    /* response so far */
    char* response;

    /* response length */
    int rlen;
} connection_attempt_t;

/**
 * @return *  1 on success otherwise, 0 */
int url2host_and_port(
    const char *url,
    char** host_o,
    char** port_o
)
{
    const char* host, *port;

    host = url;

    port = strpbrk(host, ":/");

    if (!port)
    {
        return 0;
    }

    *host_o = strndup(host,port - host);
    if (*port == ':')
    {
        port += 1;
        *port_o = strndup(port,strpbrk(port, "/") - port);
    }
    else
    {
        *port_o = NULL;
    }

    return 1;
}

static void __build_tracker_request(trackerclient_t* me, const char* url, char **request)
{
    char *info_hash_encoded;

    assert(config_get(me->cfg, "piece_length"));
    assert(config_get(me->cfg, "infohash"));
    assert(config_get(me->cfg, "my_peerid"));
    assert(config_get(me->cfg, "npieces"));

//    info_hash_encoded = url_encode(config_get(me->cfg, "infohash"), 20);
    info_hash_encoded = uri_encode(config_get(me->cfg, "infohash"));

    asprintf(request,
             "GET %s"
             "?info_hash=%s"
             "&peer_id=%s"
             "&port=%d"
             "&uploaded=%d"
             "&downloaded=%d"
             "&left=%llu"
             "&event=started"
             "&compact=1"
             " HTTP/1.0"
             "\r\n"
             "\r\n\r\n",
             url,
             info_hash_encoded,
             config_get(me->cfg,"my_peerid"),
             config_get_int(me->cfg,"pwp_listen_port"),
             0,
             0,
             (unsigned long long)config_get_int(me->cfg,"npieces") * 
                config_get_int(me->cfg,"piece_length")
             );

    free(info_hash_encoded);
}

static void __write_cb(uv_write_t* req, int status)
{
    if (status)
    {
#if 0
        uv_err_t err = uv_last_error(uv_default_loop());
        fprintf(stderr, "uv_write error: %s\n", uv_strerror(err));
#endif
        assert(0);
    }
}

int __on_httpbody(http_parser* parser, const char *p, size_t len)
{
    connection_attempt_t *ca = parser->data;

    if (0 == trackerclient_read_tracker_response(ca->tc, p, len))
    {

    }

#if 0
    while (bencode_dict_has_next(&b))
    {
        int klen;
        const char *key;
        bencode_t bkey;

        bencode_dict_get_next(&b, &bkey, &key, &klen);

        if (!strncmp(key, "failure reason", klen))
        {
            int len2;
            const char *val;

            bencode_string_value(&bkey, &val, &len2);
            printf("ERROR: tracker responded with failure: %.*s\n", len2, val);

            ca->tc->on_work_done(ca->tc->callee, 0);
        }
    }
#endif

    return 0;
}

static void __read_cb(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf)
{
    connection_attempt_t *ca = tcp->data;

    if (nread >= 0)
    {
        ca->response = realloc(ca->response, ca->rlen + nread + 1);
        memcpy(ca->response + ca->rlen, buf->base, nread);
        ca->rlen += nread;
        ca->response[ca->rlen] = 0;
    }
    else
    {
        int nparsed;
        http_parser_settings settings;
        http_parser *parser;

        memset(&settings,0,sizeof(http_parser_settings));

        settings.on_body = __on_httpbody;

        parser = malloc(sizeof(http_parser));
        parser->data = ca;
        http_parser_init(parser, HTTP_RESPONSE);
        assert(parser->data);
        nparsed = http_parser_execute(parser, &settings, ca->response, ca->rlen);

        if (parser->upgrade)
        {
          /* handle new protocol */
        }
        else if (nparsed != ca->rlen)
        {
          /* Handle error. Usually just close the connection. */
            printf("ERROR: couldn't parse http response: %s\n",
                    http_errno_description(HTTP_PARSER_ERRNO(parser)));
        }

        //assert(uv_last_error(uv_default_loop()).code == UV_EOF);
        //uv_close((uv_handle_t*)tcp, close_cb);
    }

    free(buf->base);
}

static void __alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf)
{
    buf->len = size;
    buf->base = malloc(size);
}

static void __on_connect(uv_connect_t *req, int status)
{
    connection_attempt_t *ca = req->data;
    int r;
    char *request;
    uv_buf_t buf;
    uv_write_t *write_req;

    assert(req->data);
    assert(ca->tc);

    if (status == -1)
    {
//        fprintf(stderr, "connect callback error %s\n",
//                uv_err_name(uv_last_error(uv_default_loop())));
        return;
    }

    //req->handle = req->data;
    req->handle->data = req->data;

    /*  prepare request to write */
    __build_tracker_request(ca->tc, ca->tc->uri, &request);
    buf.base = request;
    buf.len = strlen(request);

    /*  write */
    write_req = malloc(sizeof(uv_write_t));
    r = uv_write(write_req, req->handle, &buf, 1, __write_cb);
    r = uv_read_start(req->handle,
                     (uv_alloc_cb)__alloc_cb,
                     (uv_read_cb)__read_cb);
}

static void __on_resolved(uv_getaddrinfo_t *req, int status, struct addrinfo *addr)
{
    uv_tcp_t *t;
    uv_connect_t *c;

    if (status == -1)
    {
//        fprintf(stderr, "getaddrinfo callback error %s\n",
//                uv_err_name(uv_last_error(uv_default_loop())));
        return;
    }

    t = malloc(sizeof(uv_tcp_t));
    if (0 != uv_tcp_init(uv_default_loop(), t))
    {
        printf("FAILED TCP socket creation\n");
    }
    
    c = malloc(sizeof(uv_connect_t));
    c->data = req->data;

    struct sockaddr_in *sockaddr = (struct sockaddr_in*)addr->ai_addr;

    if (0 != uv_tcp_connect(c, t, *sockaddr, __on_connect))
    {
        printf("Connection failed\n");
//        fprintf(stderr, "FAILED connection creation %s\n",
//                uv_err_name(uv_last_error(uv_default_loop())));
    }

    free(req);
    uv_freeaddrinfo(addr);
}

int thttp_connect(
        void *me_,
        const char* url)
{
    trackerclient_t *me = me_;
    char *host, *port, *default_port = "80";

    connection_attempt_t *ca;
    ca = calloc(1,sizeof(connection_attempt_t));
    ca->tc = me;
    ca->rlen = 0;

    if (0 == url2host_and_port(url,&host,&port))
    {
        assert(0);
    }

    if (!port)
    {
        port = default_port;
    }

    struct addrinfo hints;
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    uv_getaddrinfo_t *req;
    req = malloc(sizeof(uv_getaddrinfo_t));
    req->data = ca;
    
    int r;

    if ((r = uv_getaddrinfo(uv_default_loop(),
                    req, __on_resolved, host, port, &hints)))
    {
#if 0
        fprintf(stderr, "getaddrinfo call error %s\n",
                uv_err_name(uv_last_error(uv_default_loop())));
        return 0;
#endif
    }

    return 1;
}

#if 0
void thttp_dispatch_from_buffer(
        void *me_ __attribute__((__unused__)),
        const unsigned char* buf,
        unsigned int len)
{
}
#endif
