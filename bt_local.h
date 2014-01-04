
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#define TRUE 1
#define FALSE 0

#include <sys/types.h>

#include "bt_interfaces.h"

#define PROTOCOL_NAME "BitTorrent protocol"
#define INFOKEY_LEN 20
#define BLOCK_SIZE 1 << 14      // 16kb
#define PWP_HANDSHAKE_RESERVERD "\0\0\0\0\0\0\0\0"
#define VERSION_NUM 1000
#define PEER_ID_LEN 20
#define INFO_HASH_LEN 20

/* peer */
typedef struct
{
    /* for network api */
    void* nethandle;

    /* 20 byte sha1 string */
    char *peer_id;
    char *ip;
    unsigned int port;

    /* peer connection */
    void* pc;

    /* message handler */
    void* mh;
} bt_peer_t;

