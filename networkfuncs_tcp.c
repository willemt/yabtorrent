
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

#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <poll.h>
#include <winsock2.h>

#include <unistd.h>     /* for sleep */

#include <fcntl.h>

#include <getopt.h>

#include "bt.h"

#include <errno.h>

#include "cbuffer.h"

typedef struct
{
    int fd;
    int inuse;
} sock_t;

typedef struct
{
    /*  who are we sending this to? */
    int sendee;
    /*  size of the message */
    int size;
} buffered_data_header_t;

typedef struct
{
    /*  connection with tracker */
    int tracker_sock;
    /*  specific entry point for peers */
    int peer_listen_sock;
    /* ports for pwp */
    sock_t *peer_socks;
    int npeer_socks;
    /*  circular buffer for buffering failed send()s */
    cbuf_t *cb;
} net_t;

static int __tcp_connect(const char *host, const char *port)
{
    int sock = 0, err;

    struct addrinfo hints, *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;        // AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    if (0 != (err = getaddrinfo(host, port, &hints, &result)))
    {
        fprintf(stderr, "getaddrinfo: %s (%s,%s)\n",
                gai_strerror(err), host, port);
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

#if 0
        if (strlen(__cfg_bound_iface) > 0)
        {
            setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, __cfg_bound_iface,
                       strlen(__cfg_bound_iface));
        }
#endif

        if (sock == -1)
            continue;

        assert(rp);
        assert(rp->ai_addr);

        if (-1 == connect(sock, rp->ai_addr, rp->ai_addrlen))
        {
            perror("could not connect");
            close(sock);
            freeaddrinfo(result);
            return 0;
        }
        else
        {
            printf("[connected to %s:%s]\n", host, port);
            freeaddrinfo(result);
            return sock;
        }
    }

    if (rp == NULL)
    {
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(result);
    return 0;
}

/**
 * @return bytes sent, -1 error, -2 disconnect */
static int __tcpsock_send(const int sock, const char *data, int len)
{
    int bytes_sent = 0;

    for (; 0 < len;)
    {
        int bytes_out, nsocks_rdy, ii;

        struct timeval select_delay;

        fd_set fdset_send;

        select_delay.tv_sec = 0;
        select_delay.tv_usec = 1000;
        FD_ZERO(&fdset_send);
        FD_SET(sock, &fdset_send);
        nsocks_rdy = select(sock + 1, NULL, &fdset_send, NULL, &select_delay);
        if (nsocks_rdy < 0)
        {
            perror("select");
            exit(0);
        }
        else if (0 == nsocks_rdy)
        {
            return bytes_sent;
        }

//        FD_CLR(sock, &fdset_send);
        assert(FD_ISSET(sock, &fdset_send));
//        if ((bytes_out = write(sock, data, len)) < 0)
//        if ((bytes_out = write(sock, data, len)) < 0)
        bytes_out = send(sock, data, len, MSG_NOSIGNAL);
        if (bytes_out < 0)
        {
            /*  other side has disconnected us */
            if (errno == EPIPE)
            {
                printf("peer disconnected!\n");
                return -2;
            }
            else
            {
                printf("errno: %s\n", strerror(errno));
                perror("writing to socket");
                assert(0);
                exit(EXIT_FAILURE);
                return -1;


            }
        }
        len -= bytes_out;
        data += bytes_out;
        bytes_sent += bytes_out;
    }

    return bytes_sent;
}

/*----------------------------------------------------------------------------*/

/*  @param my_ip: populate this with the interface's ip */

int __tcpsock_http_recv(int sock, char **rdata, int *rlen)
{
#define BUF_SIZE 2000

    char buf[BUF_SIZE + 1], *out = NULL;

    int recvd = 0;

    //MSG_DONTWAIT
#if 0
    while ((recvd = recv(sock, buf, BUF_SIZE, MSG_WAITALL)) > 0)
    {
        buf[recvd] = '\0';

        if (!out)
        {
            out = strndup(buf, recvd);
        }
        else
        {
            char *tmp = NULL;

            asprintf(&tmp, "%s%s", out, buf);
            assert(tmp);
            free(out);
            out = tmp;
        }
    }

    if (recvd == 0)
    {
        printf("Other side has shutdown\n");
        perror("can't recv from socket:");
//        exit(EXIT_FAILURE);
    }
#else

    if (0 == (recvd = recv(sock, buf, BUF_SIZE, MSG_WAITALL)))
    {
        printf("Other side has shutdown\n");
        perror("can't recv from socket:");
//        exit(EXIT_FAILURE);
        return 0;
    }
    else
    {
        buf[recvd] = '\0';
        out = malloc(recvd);
        memcpy(out, buf, recvd);

        *rdata = out;
        *rlen = recvd;
    }

#endif

    return 1;
}

/*----------------------------------------------------------------------------*/

static void __find_unused_sock(net_t * net, int *peerid)
{
    int ii;

    for (ii = 0; ii < net->npeer_socks; ii++)
    {
        if (0 == net->peer_socks[ii].inuse)
        {
            *peerid = ii;
            return;
        }
    }
    *peerid = net->npeer_socks;
    net->npeer_socks++;
}

/**
 * Add a new peer with this socket
 * @return new peer's peerid
 * */
static int __add_new_peer_with_sock(net_t * net, const int fd)
{
    int peerid;
    sock_t *sock;

    __find_unused_sock(net, &peerid);
    printf("adding new peerid: %d sock: %d\n", peerid, fd);
    net->peer_socks =
        realloc(net->peer_socks, sizeof(sock_t) * net->npeer_socks);
    sock = &net->peer_socks[peerid];
    sock->fd = fd;
    sock->inuse = 1;
    return peerid;
}

int peer_connect(void **udata, const char *host, const char *port, int *peerid)
{
    net_t *net = *udata;
    sock_t *sock;
    int fd;

    if (0 == (fd = __tcp_connect(host, port)))
    {
        return 0;
    }
    else
    {
        *peerid = __add_new_peer_with_sock(net, fd);
        return 1;
    }
}

static void __add_to_sendbuffer(cbuf_t * cb,
                                const int peerid,
                                const int size, const char *data)
{
    buffered_data_header_t hdr;

    hdr.sendee = peerid;
    hdr.size = size;
    cbuf_offer(cb, (void *) &hdr, sizeof(buffered_data_header_t));
    cbuf_offer(cb, data, hdr.size);
}

/**
 *
 * @return 0 if added to buffer due to write failure, -2 if disconnect
 */
int peer_send(void **udata,
              const int peerid, const unsigned char *send_data, const int len)
{
    net_t *net = *udata;
    sock_t *sock;

    assert(*udata);
    assert(peerid < net->npeer_socks);

//    printf("sending: %d %s\n", len, send_data);

#if 1    /*  circular buffer */

    /*  init buffer */
    if (!net->cb)
    {
        net->cb = cbuf_new(20);
    }

    /* poll from the buffer until it is empty;
     * or we have errored with the send() again */
    while (!cbuf_is_empty(net->cb))
    {
        buffered_data_header_t *hdr;

        unsigned char *data;

        int bytes_sent, diff, sendee;

        /*  poll header */
        hdr = (void *) cbuf_peek(net->cb, (int) sizeof(buffered_data_header_t));

        /*  poll header + data */
        data = cbuf_peek(net->cb, hdr->size + sizeof(buffered_data_header_t)) +
            sizeof(buffered_data_header_t);

        sendee = hdr->sendee;
        sock = &net->peer_socks[sendee];
        assert(sock != 0);
        bytes_sent = __tcpsock_send(sock->fd, data, hdr->size);
        diff = hdr->size - bytes_sent;

        if (bytes_sent < 0)
            return bytes_sent;

        if (0 < diff)
        {
            /* leave space for us to re-write a header.
             * Compiler will optimise out the extra sizeof() */
            cbuf_poll(net->cb,
                      sizeof(buffered_data_header_t) + bytes_sent -
                      sizeof(buffered_data_header_t));
            hdr = (void *) cbuf_peek(net->cb, sizeof(buffered_data_header_t));
            hdr->sendee = sendee;
            hdr->size = diff;
            break;
        }
        else
        {
            cbuf_poll(net->cb, sizeof(buffered_data_header_t) + bytes_sent);
        }
    }
#endif

    int bytes_sent, diff;

    sock = &net->peer_socks[peerid];
    assert(sock->fd != 0);
    bytes_sent = __tcpsock_send(sock->fd, send_data, len);
    if (bytes_sent < 0)
        return bytes_sent;
    diff = len - bytes_sent;

    /*  wasn't able to be sent, add to buffer */
    if (0 < diff)
    {
        __add_to_sendbuffer(net->cb, peerid, diff, send_data + (len - diff));
    }

    return 1;
}

/*  return how many we've read */
int peer_recv_len(void **udata, const int peerid, char *buf, int *len)
{
    net_t *net;
    sock_t *sock;
    int recvd = 0;

    net = *udata;
    sock = &net->peer_socks[peerid];

    recvd = recv(sock->fd, buf, *len, MSG_WAITALL);

    if (0 < recvd)
    {
        assert(recvd == *len);
    }
    else if (recvd == 0)
    {
        printf("Peer has shutdown\n");
        perror("recv from socket:");
        assert(0);
        return 0;
    }
    else
    {
//        if (errno == EAGAIN)
        perror("unable to receive from peer");
        assert(0);
        exit(EXIT_FAILURE);
        return 0;
    }

    return recvd;
}

int peer_disconnect(void **udata, int peerid)
{
    net_t *net = *udata;
    sock_t *sock;

    sock = &net->peer_socks[peerid];
    sock->fd = 0;
    sock->inuse = 0;
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
    int nsocks_rdy = 0, fd_highest, ii;

    struct timeval select_delay;

    fd_set fdset_read;

    net_t *net = *udata;

    if (!net)
        return 1;

    assert(0 < net->peer_listen_sock);

    FD_ZERO(&fdset_read);

    /* 1. add all sockets to the fd list
     * and find the higest value fd. */
    for (ii = 0, fd_highest = 0; ii < net->npeer_socks; ii++)
    {
        sock_t *sock = &net->peer_socks[ii];

        if (0 == sock->inuse)
            continue;

        FD_SET(sock->fd, &fdset_read);
        if (fd_highest < sock->fd)
            fd_highest = sock->fd;
    }

    /* 1b. add peer connection socket */
    FD_SET(net->peer_listen_sock, &fdset_read);
    if (fd_highest < net->peer_listen_sock)
        fd_highest = net->peer_listen_sock;

    int us_waited = 0;

    /*  set wait */
    select_delay.tv_sec = 0;
//    select_delay.tv_usec = 0;
    select_delay.tv_usec = msec_timeout * 1000;

    /*  2. select from sockets */
    while (us_waited < msec_timeout * 1000)
    {
        int us_wait;

        us_wait = select_delay.tv_usec;

        /*  2b. do select */
        nsocks_rdy =
            select(fd_highest + 1, &fdset_read, NULL, NULL, &select_delay);

        if (nsocks_rdy < 0)
        {
            perror("select");
            exit(0);
        }
        /* 2c. Exit, we've found a socket with data waiting */
        else if (0 < nsocks_rdy)
        {
            break;
        }
//        printf("us_waited: %d %d\n", us_waited, msec_timeout);
        us_waited += us_wait - select_delay.tv_usec;
        select_delay.tv_usec = msec_timeout * 1000;
    }

    if (nsocks_rdy == 0)
        return 1;

//    printf("nsocks_rdy %d\n", nsocks_rdy);

    /* 3. read from sockets */
    for (ii = 0; ii < net->npeer_socks; ii++)
    {
        sock_t *sock;

        sock = &net->peer_socks[ii];

        /*  must be inuse and part of the read set */
        if (1 == sock->inuse && FD_ISSET(sock->fd, &fdset_read))
        {
            /*  3a. read! */
            func_process(data, ii);
            FD_CLR(sock->fd, &fdset_read);
        }
    }

    /*  4. does the peer_listen_sock have traffic?  */
    if (FD_ISSET(net->peer_listen_sock, &fdset_read))
    {
        socklen_t clilen;

        struct sockaddr_in cli_addr;

        int new_fd;

        clilen = sizeof(cli_addr);
        new_fd =
            accept4(net->peer_listen_sock, (struct sockaddr *) &cli_addr,
                    &clilen, SOCK_NONBLOCK);
        if (new_fd < 0)
        {
            perror("ERROR on accept");
        }

        unsigned char ip[32], *val;

        int port;

        port = ntohs(cli_addr.sin_port);
        val = (void *) &cli_addr.sin_addr.s_addr;

        sprintf(ip, "%d.%d.%d.%d", val[0], val[1], val[2], val[3]);
//        printf("\n\n\n\nconnection from sock: %d %s:%d\n", new_fd, ip, port);
        func_process_connection(data, __add_new_peer_with_sock(net, new_fd),
                                ip, port);
    }

    return 1;
}

/*----------------------------------------------------------------------------*/

/**
 * open up to listen to peers */
int peer_listen_open(void **udata, const int port)
{
    net_t *net = *udata;

    struct sockaddr_in serv_addr;

    /*  open TCP socket */
    if ((net->peer_listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        error("ERROR opening socket");
    }

    /*  configure TCP socket */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    /*  bind for listening */
    if (bind(net->peer_listen_sock,
             (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
    }

    /*  listen */
    if (-1 == listen(net->peer_listen_sock, 10))
    {
        perror("listen not working");
    }

//    unsigned char *ip;
//    ip = (void *) &serv_addr.sin_addr.s_addr;
//    printf("ip: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    return 1;
}

/*----------------------------------------------------------------------------*/

#if 0
bt_net_pwp_funcs_t pwpNetFuncs = {
    peer_connect,
    peer_send,
    peer_recv_len,
    peer_disconnect,
    peers_poll,
    peer_listen_open
};
#endif
