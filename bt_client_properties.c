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
//#include <sys/resource.h>

#include <stdarg.h>

//#include <arpa/inet.h>

#include "pwp_connection.h"

#include "bitfield.h"
#include "event_timer.h"

#include "bt.h"
#include "bt_local.h"
#include "bt_block_readwriter_i.h"
#include "bt_piece_db.h"
#include "bt_string.h"

#include "bt_client_private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * Set the failure flag to true
 */
void bt_client_set_failed(void *bto, const char *reason)
{

}

void* bt_client_get_config(void *bto)
{
    bt_client_t *bt = bto;

    return bt->cfg;
}

/**
 * Set the client's peer id
 */
void bt_client_set_peer_id(void *bto, char *peer_id)
{
    bt_client_t *bt = bto;
    int ii;

    bt->my_peer_id = peer_id;

#if 0
    for (ii = 0; ii < bt_peermanager_count(bt->pm); ii++)
    {
        bt_peerconn_set_my_peer_id(bt->peerconnects[ii], peer_id);
    }
#endif
}

/**
 * Get the client's peer id
 */
char *bt_client_get_peer_id(void *bto)
{
    bt_client_t *bt = bto;

    return bt->my_peer_id;
}

/**
 * Set the logging function
 */
void bt_client_set_logging(void *bto, void *udata, func_log_f func)
{
    bt_client_t *bt = bto;

    bt->func_log = func;
    bt->log_udata = udata;
}

/**
 * Set network functions
 */
void bt_client_set_pwp_net_funcs(void *bto, bt_net_pwp_funcs_t * net)
{
    bt_client_t *bt = bto;

    memcpy(&bt->net, net, sizeof(bt_net_pwp_funcs_t));
}

/**
 * @return number of peers this client is involved with
 */
int bt_client_get_num_peers(void *bto)
{
    bt_client_t *bt = bto;

    return bt_peermanager_count(bt->pm);
}

/**
 * @return number of pieces in this torrent
 */
int bt_client_get_num_pieces(void *bto)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get_length(bt->db);
}

/**
 * @return the total size of the file as specified by the torrent
 * @see bt_client_read_metainfo_file
 */
int bt_client_get_total_file_size(void *bto)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get_tot_file_size(bt->db);
}

/**
 * @return reason for failure
 */
char *bt_client_get_fail_reason(void *bto)
{
    bt_client_t *bt = bto;

    return bt->fail_reason;
}

/**
 * @return number of bytes downloaded
 */
int bt_client_get_nbytes_downloaded(void *bto)
{
    //FIXME_STUB;
    return 0;
}

/**
 * @todo implement
 * @return 1 if client has failed 
 */
int bt_client_is_failed(void *bto)
{
    return 0;
}

/* @} ------------------------------------------------------------------------*/

