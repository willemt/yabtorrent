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
#include "bt_filedumper.h"
#include "bt_diskcache.h"
//#include "bt_piece_i.h"
#include "bt_piece_db.h"
//#include "bt_peer_connection.h"
#include "bt_string.h"

#include "bt_client_private.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>



/* @} ------------------------------------------------------------------------*/
/** @name Properties
 * @{ */

/**
 * Set the maximum number of peer connection
 */
void bt_client_set_max_peer_connections(void *bto, const int npeers)
{
    bt_client_t *bt = bto;

    bt->cfg.max_peer_connections = npeers;
}

/**
 * Set timeout in milliseconds
 */
void bt_client_set_select_timeout_msec(void *bto, const int val)
{
    bt_client_t *bt = bto;

    bt->cfg.select_timeout_msec = val;
}

/**
 * Set maximum amount of megabytes used by piece cache
 */
void bt_client_set_max_cache_mem(void *bto, const int val)
{
    bt_client_t *bt = bto;

    bt->cfg.max_cache_mem = val;
}

/**
 * Set the number of completed peers
 */
void bt_client_set_num_complete_peers(void *bto, const int npeers)
{
    bt_client_t *bt = bto;

    bt->ncomplete_peers = npeers;
}

/**
 * Set the failure flag to true
 */
void bt_client_set_failed(void *bto, const char *reason)
{

}

/**
 * Set the client's peer id
 */
void bt_client_set_peer_id(void *bto, char *peer_id)
{
    bt_client_t *bt = bto;
    int ii;

    bt->cfg.p_peer_id = peer_id;
    for (ii = 0; ii < bt->npeers; ii++)
    {
        bt_peerconn_set_their_peer_id(bt->peerconnects[ii], peer_id);
    }
}

/**
 * Get the client's peer id
 */
char *bt_client_get_peer_id(void *bto)
{
    bt_client_t *bt = bto;

    return bt->cfg.p_peer_id;
}

/**
 * How many pieces are there of this file?
 *
 * <b>Note:</b> bt_read_torrent_file sets this
 * The size of a piece is determined by the publisher of the torrent. A good recommendation is to use a piece size so that the metainfo file does not exceed 70 kilobytes.
 */
void bt_client_set_npieces(void *bto, int npieces)
{
    assert(FALSE);
// fixed_piece_size = size_of_torrent / number_of_pieces
    bt_client_t *bt = bto;

    bt->cfg.pinfo.npieces = npieces;
}

/**
 * Set file download path
 * @see bt_client_read_metainfo_file
 */
void bt_client_set_path(void *bto, const char *path)
{
    bt_client_t *bt = bto;

    bt->path = strdup(path);
    bt_filedumper_set_cwd(bt->fd, bt->path);
}

/**
 * Set piece length
 * @see bt_client_read_metainfo_file
 */
void bt_client_set_piece_length(void *bto, const int len)
{
    bt_client_t *bt = bto;

    bt->cfg.pinfo.piece_len = len;
    bt_piecedb_set_piece_length(bt->db, len);
    bt_filedumper_set_piece_length(bt->fd, len);
    bt_diskcache_set_size(bt->dc, len);
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
 * Set shutdown when completed flag
 *
 * If this is set, the client will shutdown when the download is completed.
 *
 * @return 1 on success, 255 if the option doesn't exist, 0 otherwise
 * */
void bt_client_set_opt_shutdown_when_completed(void *bto, const int flag)
{
    bt_client_t *bt = bto;

    bt->cfg.o_shutdown_when_complete = flag;
}

/**
 * Set a key-value option
 *
 * Options include:
 * pwp_listen_port - The port the client will listen on for PWP connections
 * tracker_interval - Length in seconds that client will communicate with the tracker
 * tracker_url - Set tracker's url
 * infohash - Set the client's infohash
 *
 * @param key The option
 * @param val The value
 * @param val_len Value's length
 *
 */
int bt_client_set_opt(void *bto,
                      const char *key, const char *val, const int val_len)
{
    bt_client_t *bt = bto;

    if (!strcmp(key, "pwp_listen_port"))
    {
        bt->cfg.pwp_listen_port = atoi(val);
        return 1;
    }
#if 0
    else if (!strcmp(key, "tracker_interval"))
    {
        bt->interval = atoi(val);
        return 1;
    else if (!strcmp(key, "tracker_url"))
    {
        bt->cfg.tracker_url = calloc(1, sizeof(char) * (val_len + 1));
        strncpy(bt->cfg.tracker_url, val, val_len);
        return 1;
    }
#endif
    else if (!strcmp(key, "infohash"))
    {
        bt->cfg.info_hash = strdup(val);
        return 1;
    }
    else if (!strcmp(key, "tracker_backup"))
    {

        return 1;
    }

    return 255;
}

/**
 * Get a key-value option
 *
 * @return string of value if applicable, otherwise NULL
 */
char *bt_client_get_opt_string(void *bto, const char *key)
{
    bt_client_t *bt = bto;

#if 0
    if (!strcmp(key, "tracker_url"))
    {
        return bt->cfg.tracker_url;
    }
    else
#endif
    if (!strcmp(key, "infohash"))
    {
        return bt->cfg.info_hash;
    }
    else
    {
        return NULL;
    }
}

/**
 * Get a key-value option
 *
 * @return integer of value if applicable, otherwise -1
 */
int bt_client_get_opt_int(void *bto, const char *key)
{
    bt_client_t *bt = bto;

    if (!strcmp(key, "pwp_listen_port"))
    {
        return bt->cfg.pwp_listen_port;
    }
#if 0
    else if (!strcmp(key, "tracker_interval"))
    {
        return bt->interval;
    }
#endif
    else
    {
        return -1;
    }
}

/**
 * @return number of peers this client is involved with
 */
int bt_client_get_num_peers(void *bto)
{
    bt_client_t *bt = bto;

    return bt->npeers;
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

