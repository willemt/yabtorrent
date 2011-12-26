
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

/*----------------------------------------------------------------------------*/
typedef void (
    *func_log_f
)    (
    void *udata,
    void *src,
//    bt_peer_t * peer,
    const char *buf,
    ...
);

typedef bt_piece_t *(
    *func_getpiece_f
)          (
    void *udata,
    int piece
);

typedef int (
    *func_pollblock_f
)   (
    void *udata,
    bt_bitfield_t * peers_bitfield,
    bt_block_t * blk
);

typedef int (
    *func_have_f
)   (
    void *udata,
    bt_peer_t * peer,
    int piece
);

typedef int (
    *func_push_block_f
)   (
    void *udata,
    bt_peer_t * peer,
    bt_block_t * block,
    void *data
);

typedef int (
    *func_send_f
)   (
    void *udata,
    bt_peer_t * peer,
    void *send_data,
    const int len
);

typedef char *(
    *func_get_infohash_f
)    (
    void *udata,
    bt_peer_t * peer
);

typedef int (
    *func_recv_f
)   (
    void *udata,
    bt_peer_t * peer,
    char *buf,
    int *len
);

typedef int (
    *func_disconnect_f
)   (
    void *udata,
    bt_peer_t * peer,
    char *reason
);

typedef int (
    *func_connect_f
)   (
    void *bto,
    void *pc,
    bt_peer_t * peer
);

#if 0
/*  bt logger */
typedef struct
{
    void *udata;
    void (
    *func
    )    (
    void *,
    char *
    );
} bt_logger_t;
#endif

/*----------------------------------------------------------------------------*/
/*  send/receiver */
typedef struct
{
    func_send_f send;
    func_recv_f recv;
    func_disconnect_f disconnect;
    func_connect_f connect;
} sendreceiver_i;

typedef int (
    *func_write_block_f
)   (
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
);

typedef void *(
    *func_read_block_f
)    (
    void *udata,
    void *caller,
    const bt_block_t * blk
);

/*----------------------------------------------------------------------------*/
typedef int (
    *func_object_get_int_f
)   (
    void *
);

typedef int (
    *func_get_int_f
)   (
    void *,
    void *pr
);

typedef int (
    *func_get_int_const_f
)   (
    const void *,
    const void *pr
);

/*----------------------------------------------------------------------------*/
/*--Choker interface----------------------------------------------------------*/
typedef void *(
    *func_new_choker_f
)    (
    const int max_unchoked_connections
);

typedef void (
    *func_mutate_object_f
)    (
    void *,
    void *
);

typedef void (
    *func_mutate_object_with_int_f
)    (
    void *,
    int
);

typedef int (
    *func_collection_count_f
)   (
    void *
);

typedef void (
    *func_mutate_f
)    (
    void *
);


/*  commands that chokers use for querying/modifiying peers */
typedef struct
{
    func_get_int_const_f get_drate;

    func_get_int_const_f get_urate;

    func_get_int_f get_is_interested;

    func_mutate_object_f choke_peer;

    func_mutate_object_f unchoke_peer;
} bt_choker_peer_i;

typedef void (
    *func_choker_set_peer_iface
)    (
    void *ckr,
    void *udata,
    bt_choker_peer_i * iface
);



/*  chocker */
typedef struct
{
    func_new_choker_f new;

    func_mutate_object_f remove_peer;

    func_mutate_object_f add_peer;

    func_mutate_object_f unchoke_peer;

    func_collection_count_f get_npeers;

    func_choker_set_peer_iface set_choker_peer_iface;

    func_mutate_f decide_best_npeers;
} bt_choker_i;


/*----------------------------------------------------------------------------*/

typedef void *(
    *func_piece_selector_new_f
)    (
    int size
);

typedef void (
    *func_add_piece_back_f
)    (
    void *self,
    int idx
);

typedef void (
    *func_announce_peer_have_piece_f
)    (
    void *self,
    void *peer,
    int idx
);

typedef int (
    *func_get_best_piece_from_peer_f
)   (
    void *,
    const void *peer
);



/*  piece selector interface */
typedef struct
{
    func_piece_selector_new_f new;

    /*  add a piece back to the selector */
    func_add_piece_back_f offer_piece;

    func_mutate_object_with_int_f announce_have_piece;

    func_mutate_object_f remove_peer;

    func_mutate_object_f add_peer;

    func_announce_peer_have_piece_f announce_peer_have_piece;

    func_collection_count_f get_npeers;

    func_collection_count_f get_npieces;

    /* remove best piece */
    func_get_best_piece_from_peer_f poll_best_piece_from_peer;
} bt_pieceselector_i;

/*----------------------------------------------------------------------------*/

#if 0
/*  piecepoller */
typedef struct
{
    func_pollpiece_f poll_piece;
} bt_piecepoller_i;


/*  piecegetter */
typedef struct
{
    func_getpiece_f get_piece;
} bt_piecegetter_i;

/*  piece2file mapper */
typedef struct
{
    func_get_filename_from_block_f get_filename_from_block;
} bt_piece2file_mapper_i;
#endif

/*  logger */
typedef struct
{
    func_log_f log;
} bt_logger_i;

/*  block writer/reader */
typedef struct
{
    func_write_block_f write_block;

    func_read_block_f read_block;

    /*  release this block from the holder of it */
//    func_giveup_block_f giveup_block;
} bt_blockrw_i;
