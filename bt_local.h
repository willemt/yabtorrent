
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


typedef unsigned char byte;

#include <stdbool.h>
#include <sys/types.h>

#define TRUE 1
#define FALSE 0

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
    /* 20 byte sha1 string */
    char *peer_id;
    char *ip;
    unsigned int port;

    /* for network api */
    void* nethandle;

    /* peer connection */
    void* pc;

    /* message handler */
    void* mh;

} bt_peer_t;

/*----------------------------------------------------------------------------*/

char *read_file(const char *name, int *len);

/*----------------------------------------------------------------------------*/

void *bt_rarestfirst_selector_new(int npieces);

void bt_rarestfirst_selector_offer_piece(void *r, int piece_idx);

void bt_rarestfirst_selector_announce_have_piece(void *r, int piece_idx);

void bt_rarestfirst_selector_remove_peer(void *r, void *peer);

void bt_rarestfirst_selector_add_peer(void *r, void *peer);

void bt_rarestfirst_selector_announce_peer_have_piece(void *r,
                                                      void *peer,
                                                      int piece_idx);

int bt_rarestfirst_selector_get_npeers(void *r);


int bt_rarestfirst_selector_get_npieces(void *r);

int bt_rarestfirst_selector_poll_best_piece(void *r, const void *peer);

/*----------------------------------------------------------------------------*/

void *bt_leeching_choker_new(const int size);

void bt_leeching_choker_add_peer(void *ckr, void *peer);

void bt_leeching_choker_remove_peer(void *ckr, void *peer);

void bt_leeching_choker_announce_interested_peer(void *cho, void *peer);

void bt_leeching_choker_decide_best_npeers(void *ckr);

void bt_leeching_choker_optimistically_unchoke(void *ckr);

void bt_leeching_choker_unchoke_peer(void *ckr, void *peer);

int bt_leeching_choker_get_npeers(void *ckr);

void bt_leeching_choker_set_choker_peer_iface(void *ckr,
                                              void *udata,
                                              bt_choker_peer_i * iface);

void bt_leeching_choker_get_iface(bt_choker_i * iface);

/*----------------------------------------------------------------------------*/
void *bt_seeding_choker_new(const int size);

void bt_seeding_choker_add_peer(void *ckr, void *peer);

void bt_seeding_choker_remove_peer(void *ckr, void *peer);

void bt_seeding_choker_decide_best_npeers(void *ckr);

void bt_seeding_choker_unchoke_peer(void *ckr, void *peer);

void bt_seeding_choker_set_choker_peer_iface(void *ckr,
                                             void *udata,
                                             bt_choker_peer_i * iface);

int bt_seeding_choker_get_npeers(void *ckr);

void bt_seeding_choker_get_iface(bt_choker_i * iface);

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/

