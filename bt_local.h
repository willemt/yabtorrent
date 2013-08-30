
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


//typedef unsigned char byte;

#define TRUE 1
#define FALSE 1

#include <sys/types.h>

typedef struct
{
    void *private;
} bt_piececache_t;

/* disk cache */
typedef struct
{
    int pass;
} bt_diskcache_t;

#include "bt_interfaces.h"

/*----------------------------------------------------------------------------*/
#define PROTOCOL_NAME "BitTorrent protocol"
#define INFOKEY_LEN 20
#define BLOCK_SIZE 1 << 14      // 16kb
#define PWP_HANDSHAKE_RESERVERD "\0\0\0\0\0\0\0\0"
#define VERSION_NUM 1000
#define PEER_ID_LEN 20
#define INFO_HASH_LEN 20

/*----------------------------------------------------------------------------*/

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

