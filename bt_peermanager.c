/**
 * @file
 * @brief Major class tasked with managing downloads
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include <sys/time.h>

#include <stdarg.h>

#include "pwp_connection.h"

#include "bitfield.h"
#include "event_timer.h"

#include "bt.h"
#include "bt_local.h"
#include "bt_block_readwriter_i.h"
#include "bt_filedumper.h"
#include "bt_diskcache.h"
#include "bt_piece_db.h"
#include "bt_string.h"

#include "bt_client_private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

typedef struct {


} bt_peermanager_t;

int bt_peermanager_contains(void *bto, const char *ip, const int port)
{
    bt_client_t *bt = bto;
    int ii;

    for (ii = 0; ii < bt->npeers; ii++)
    {
        bt_peer_t *peer;

        peer = bt_peerconn_get_peer(bt->peerconnects[ii]);
        if (!strcmp(peer->ip, ip) && atoi(peer->port) == port)
        {
            return 1;
        }
    }
    return 0;
}

static void *__netpeerid_to_peerconn(bt_client_t * bt, const int netpeerid)
{
    int ii;

    for (ii = 0; ii < bt->npeers; ii++)
    {
        void *pc;
        bt_peer_t *peer;

        pc = bt->peerconnects[ii];

//        if (!bt_peerconn_is_active(pc))
//            continue;
        peer = bt_peerconn_get_peer(pc);
        if (peer->net_peerid == netpeerid)
            return pc;
    }

    assert(FALSE);
    return NULL;
}

/**
 * Add the peer.
 * Initiate connection with 
 *
 * @return freshly created bt_peer
 */
void *bt_peermanager_add_peer(void *bto,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port)
{
    bt_peermanager_t *me = bto;
    void *pc;
    bt_peer_t *peer;

    /*  peer is me.. */
    if (!strncmp(ip, me->cfg.my_ip, ip_len) && port == me->cfg.pwp_listen_port)
    {
        return NULL;
    }
    /* prevent dupes.. */
    else if (__have_peer(me, ip, port))
    {
        return NULL;
    }

    peer = calloc(1, sizeof(bt_peer_t));

    /*  'compact=0'
     *  doesn't use peerids.. */
    if (peer_id)
    {
        asprintf(&peer->peer_id, "00000000000000000000");
    }
    else
    {
        asprintf(&peer->peer_id, "", peer_id_len, peer_id);
    }
    asprintf(&peer->ip, "%.*s", ip_len, ip);
    asprintf(&peer->port, "%d", port);

    /* create a peer connection for this peer */
    pwp_connection_functions_t funcs = {
        .send = __FUNC_peerconn_send_to_peer,
        .recv = __FUNC_peerconn_recv_from_peer,
        .pushblock = __FUNC_peercon_pushblock,
        .pollblock = __FUNC_peercon_pollblock,
        .disconnect = __FUNC_peerconn_disconnect,
        .connect = __FUNC_peerconn_connect,
        .getpiece = bt_client_get_piece,
        .write_block_to_stream = __FUNC_peerconn_write_block_to_stream,
        .piece_is_complete = __FUNC_peerconn_pieceiscomplete,
    };

    pc = bt_peerconn_new();
    bt_peerconn_set_piece_info(pc,me->cfg.pinfo.npieces,me->cfg.pinfo.piece_len);
    bt_peerconn_set_peer(pc, peer);
    bt_peerconn_set_their_peer_id(pc, me->cfg.p_peer_id);
    bt_peerconn_set_infohash(pc, me->cfg.info_hash);

    __log(bto,NULL,"adding peer: ip:%.*s port:%d\n", ip_len, ip, port);

    me->npeers++;
    me->peerconnects = realloc(me->peerconnects, sizeof(void *) * me->npeers);
    me->peerconnects[me->npeers - 1] = pc;
    me_leeching_choker_add_peer(me->lchoke, pc);
    return peer;
}

/**
 * Remove the peer.
 * Disconnect the peer
 *
 * @todo add disconnection functionality
 *
 * @return 1 on sucess; otherwise 0
 */
int bt_peermanager_remove_peer(void *bto, const int peerid)
{
//    bt_client_t *bt = bto;
//    bt->peerconnects[bt->npeers - 1]

//    bt_leeching_choker_add_peer(bt->lchoke, peer);
    return 1;
}


void* bt_peermanager_new()
{
    bt_peermanager_t* me;

    me = calloc(1,sizeof(bt_peermanager_t));
}
