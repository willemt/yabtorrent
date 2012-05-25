
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
 *
 * @section description
 * Piece database's role: holding pieces
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <arpa/inet.h>

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bt.h"
#include "bt_local.h"


typedef struct
{
    int npieces;
    bt_piece_t **pieces;
    int piece_length_bytes;
    int tot_file_size_bytes;
//    bt_filelist_t *fl;

    /*  reader and writer of blocks to disk */
    bt_blockrw_i *blockrw;
    void *blockrw_data;
} bt_piecedb_private_t;

#define priv(x) ((bt_piecedb_private_t*)(x))

/*----------------------------------------------------------------------------*/

bt_piecedb_t *bt_piecedb_new()
{
    bt_piecedb_t *db;

    db = calloc(1, sizeof(bt_piecedb_private_t));
    priv(db)->tot_file_size_bytes = 0;

    return db;
}

#if 1
void bt_piecedb_set_piece_length(bt_piecedb_t * db,
                                 const int piece_length_bytes)
{
    priv(db)->piece_length_bytes = piece_length_bytes;
//    printf("piece length: %d\n", priv(db)->piece_length_bytes);
}

void bt_piecedb_set_tot_file_size(bt_piecedb_t * db,
                                  const int tot_file_size_bytes)
{
//    printf("setting tot file size: %d\n", tot_file_size_bytes);
    priv(db)->tot_file_size_bytes = tot_file_size_bytes;
}

int bt_piecedb_get_tot_file_size(bt_piecedb_t * db)
{
    return priv(db)->tot_file_size_bytes;
}

void bt_piecedb_set_diskstorage(bt_piecedb_t * db,
                                bt_blockrw_i * irw, void *udata)
{
    assert(irw->write_block);
    assert(irw->read_block);
    assert(udata);


    priv(db)->blockrw = irw;
    priv(db)->blockrw_data = udata;
}
#endif

#if 0
void bt_piecedb_set_piece_info(bt_piecedb_t * db, bt_piece_info_t * pinfo)
{
    priv(db)->pinfo = pinfo;
}
#endif

/*
 * get the best piece to download from this bitfield
 */
bt_piece_t *bt_piecedb_poll_best_from_bitfield(bt_piecedb_t * db,
                                               bt_bitfield_t * bf_possibles)
{
    int ii;

    for (ii = 0; ii < priv(db)->npieces; ii++)
    {
        if (!bt_bitfield_is_marked(bf_possibles, ii))
            continue;

        if (!bt_piece_is_fully_requested(priv(db)->pieces[ii]))
        {
            return priv(db)->pieces[ii];
        }
    }

    return NULL;
}

bt_piece_t *bt_piecedb_get(bt_piecedb_t * db, const int idx)
{
//    assert(idx < priv(db)->npieces);
    if (priv(db)->npieces == 0)
        return NULL;

    assert(0 <= idx);
    return priv(db)->pieces[idx];
}

/*  take care of the situation where the last piece is size differently */
static int __figure_out_new_piece_size(bt_piecedb_t * db)
{
    int tot_bytes_used = 0, ii;

    /*  figure out current total size */
    for (ii = tot_bytes_used = 0; ii < priv(db)->npieces; ii++)
        tot_bytes_used += bt_piece_get_size(priv(db)->pieces[ii]);

//    printf("%d %d\n", bt_piecedb_get_tot_file_size(db), tot_bytes_used);

    if (bt_piecedb_get_tot_file_size(db) - tot_bytes_used <
        priv(db)->piece_length_bytes)
    {
        return bt_piecedb_get_tot_file_size(db) - tot_bytes_used;
    }
    else
    {
        return priv(db)->piece_length_bytes;
    }
}

/*  add a piece with this sha1sum */
void bt_piecedb_add(bt_piecedb_t * db, const char *sha1)
{
    bt_piece_t *pce;

//    pce = malloc(sizeof(bt_piece_t));
    int size = __figure_out_new_piece_size(db);

//    printf("adding piece: %d bytes %d\n", size, priv(db)->tot_file_size_bytes);
    priv(db)->npieces += 1;
    priv(db)->pieces =
        realloc(priv(db)->pieces, sizeof(bt_piece_t *) * priv(db)->npieces);
    pce = bt_piece_new(sha1, size);
    priv(db)->pieces[priv(db)->npieces - 1] = pce;
//    bt_piece_new(piece, sha1, priv(db)->pinfo->piece_len);
    bt_piece_set_idx(pce, priv(db)->npieces - 1);
    bt_piece_set_disk_blockrw(pce, priv(db)->blockrw, priv(db)->blockrw_data);
}

int bt_piecedb_get_length(bt_piecedb_t * db)
{
    return priv(db)->npieces;
}

/*----------------------------------------------------------------------------*/
/**
 * print a string of all the downloaded pieces */
void bt_piecedb_print_pieces_downloaded(bt_piecedb_t * db)
{
    int ii, counter, is_complete = 1;

    printf("pieces downloaded: ");

    for (ii = 0, counter = 20;
         ii < bt_piecedb_get_length(db); ii++, counter += 1)
    {
        bt_piece_t *pce;

        pce = bt_piecedb_get(db, ii);

        if (bt_piece_is_complete(pce))
        {
            printf("1");
        }
        else
        {
            printf("0");
            is_complete = 0;
        }

        if (counter == 80)
        {
            counter = 2;
            printf("\n  ");
        }
    }
    printf("\n");
}
