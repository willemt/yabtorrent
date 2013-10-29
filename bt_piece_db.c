/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Keep track of what pieces we've downloaded
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 * @section description
 * Piece database's role: holding pieces
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include <stdbool.h>

#include "block.h"

#include "bitfield.h"
#include "bt.h"
#include "bt_piece_db.h"
#include "bt_piece.h"
#include "bt_local.h"
#include "bt_block_readwriter_i.h"


typedef struct
{
    int pieces_size;
    int npieces;
    bt_piece_t **pieces;
    int piece_length_bytes;
    int tot_file_size_bytes;

    /*  reader and writer of blocks to disk */
    bt_blockrw_i *blockrw;
    void *blockrw_data;

    func_add_file_f func_addfile;
} bt_piecedb_private_t;

#define priv(x) ((bt_piecedb_private_t*)(x))

bt_piecedb_t *bt_piecedb_new()
{
    bt_piecedb_t *db;

    db = calloc(1, sizeof(bt_piecedb_private_t));
    priv(db)->tot_file_size_bytes = 0;
    priv(db)->pieces_size = 999;
    priv(db)->pieces = malloc(sizeof(bt_piece_t *) * priv(db)->pieces_size);

    return db;
}

#if 0
int bt_piecedb_get_length(bt_piecedb_t * db)
{
    return priv(db)->tot_file_size_bytes;
}
#endif

void bt_piecedb_set_piece_length(bt_piecedb_t * db,
                                 const int piece_length_bytes)
{
    priv(db)->piece_length_bytes = piece_length_bytes;
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
                                bt_blockrw_i * irw,
                                func_add_file_f func_addfile,
                                void *udata)
{
    assert(irw->write_block);
    assert(irw->read_block);
    assert(udata);

    priv(db)->blockrw = irw;
    priv(db)->func_addfile = func_addfile;
    priv(db)->blockrw_data = udata;
}

void* bt_piecedb_get_diskstorage(bt_piecedb_t * db)
{
    assert(db);
    return priv(db)->blockrw_data;
}

#if 0
/**
 * Get the best piece to download from this bitfield
 */
void *bt_piecedb_poll_best_from_bitfield(void * dbo, void * bf_possibles)
{
    bt_piecedb_t* db = dbo;
    int ii;

#if 0 /*  debugging */
    printf("polling best %d %d\n",
            priv(db)->npieces, bitfield_get_length(bf_possibles));
#endif

    for (ii = 0; ii < priv(db)->npieces; ii++)
    {
        if (!bitfield_is_marked(bf_possibles, ii))
            continue;

        if (bt_piece_is_complete(priv(db)->pieces[ii]))
            continue;

        if (!bt_piece_is_fully_requested(priv(db)->pieces[ii]))
        {
            return priv(db)->pieces[ii];
        }
    }

    return NULL;
}
#endif

/**
 * Obtain this piece from the piece database
 * @return piece specified by piece_idx; otherwise NULL
 */
void *bt_piecedb_get(void* dbo, const unsigned int idx)
{
    bt_piecedb_t * db = dbo;

//    assert(idx < priv(db)->npieces);
    if (priv(db)->npieces == 0)
        return NULL;

    assert(0 <= idx);
    return priv(db)->pieces[idx];
}

/** 
 * Take care of the situation where the last piece is sized differently */
static int __figure_out_new_piece_size(bt_piecedb_t * db)
{
    int tot_bytes_used = 0, ii;


    /*  figure out current total size */
    for (ii = tot_bytes_used = 0; ii < priv(db)->npieces; ii++)
    {
        tot_bytes_used += bt_piece_get_size(priv(db)->pieces[ii]);
    }

    //printf("%d %d\n", bt_piecedb_get_tot_file_size(db), tot_bytes_used);
    //    return priv(db)->piece_length_bytes;

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

/**
 * Add a piece with this sha1sum */
void bt_piecedb_add(bt_piecedb_t * db, const char *sha1)
{
    bt_piece_t *pce;
    int size;

    if (0 == (size = __figure_out_new_piece_size(db)))
    {
        /* check if we have enough file space for this piece */
        return;
    }

#if 0 /* debugging */
    printf("adding piece: %d bytes %d piecelen:%d\n",
            size, priv(db)->tot_file_size_bytes, priv(db)->piece_length_bytes);
    {
        int ii;

        for (ii=0; ii<20; ii++)
            printf("%02x ", ((unsigned char*)sha1)[ii]);
        printf("\n");
    }
#endif

    priv(db)->npieces += 1;

    assert(priv(db)->pieces);

    /*  double capacity */
    if (priv(db)->pieces_size <= priv(db)->npieces)
    {
        priv(db)->pieces_size *= 2;
        priv(db)->pieces =
            realloc(priv(db)->pieces, sizeof(bt_piece_t *) * priv(db)->pieces_size);
    }

    assert(priv(db)->pieces);

    /* create piece */
    pce = bt_piece_new((const unsigned char*)sha1, size);
    bt_piece_set_idx(pce, priv(db)->npieces - 1);
    bt_piece_set_disk_blockrw(pce, priv(db)->blockrw, priv(db)->blockrw_data);

    /* register piece */
    priv(db)->pieces[priv(db)->npieces - 1] = pce;
}

/**
 * Mass add pieces to the piece database
 *
 * @param pieces A string of 20 byte sha1 hashes. Is always a multiple of 20 bytes in length. 
 * @param len: total length of combine hash string
 * */
void bt_piecedb_add_all(bt_piecedb_t * db, const char *pieces, const int len)
{
    int prog;

    for (prog = 1; prog <= len; prog++)
    {
        if (0 != prog % 20) continue;

        bt_piecedb_add(db, pieces);
        pieces += 20;
    }
}

/**
 * @return number of pieces downloaded */
int bt_piecedb_get_num_downloaded(bt_piecedb_t * db)
{
    int ii;
    int downloaded = 0;

    for (ii = 0; ii < bt_piecedb_get_length(db); ii++)
    {
        bt_piece_t *pce;
        
        pce = bt_piecedb_get(db, ii);

        if (bt_piece_is_downloaded(pce))
        {
            downloaded += 1;
        }
    }

    return downloaded;

}

/**
 * @return number of pieces downloaded */
int bt_piecedb_get_num_completed(bt_piecedb_t * db)
{
    int ii;
    int cnt = 0;

    for (ii = 0; ii < bt_piecedb_get_length(db); ii++)
    {
        bt_piece_t *pce;
        
        pce = bt_piecedb_get(db, ii);

        if (bt_piece_is_complete(pce))
        {
            cnt += 1;
        }
    }

    return cnt;
}

/**
 * @return 1 if all complete, 0 otherwise */

int bt_piecedb_get_length(bt_piecedb_t * db)
{
    return priv(db)->npieces;
}

/**
 * @return 1 if all complete, 0 otherwise */
int bt_piecedb_all_pieces_are_complete(bt_piecedb_t* db)
{
    int ii;

    for (ii = 0; ii < bt_piecedb_get_length(db); ii++)
    {
        bt_piece_t *pce;
        
        pce = bt_piecedb_get(db, ii);

        if (!bt_piece_is_complete(pce))
        {
            return 0;
        }
    }

    return 1;
}

/**
 * Increase total file size by this file's size
 */
void bt_piecedb_increase_piece_space(bt_piecedb_t* db, const int size)
{

    bt_piecedb_set_tot_file_size(db,
            bt_piecedb_get_tot_file_size(db) + size);
}

/**
 * Validate the client's already downloaded pieces */
void bt_piecedb_validate_downloaded_pieces(void* bto)
{
//    bt_piecedb_print_pieces_downloaded();
}

/**
 * print a string of all the downloaded pieces */
void bt_piecedb_print_pieces_downloaded(bt_piecedb_t * db)
{
    int ii, depth, is_complete = 1;

    printf("pieces (%d) downloaded: \n  ", bt_piecedb_get_length(db));

    for (ii = 0, depth = 3;
         ii < bt_piecedb_get_length(db); ii++, depth += 1)
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

        if (depth == 80)
        {
            depth = 2;
            printf("\n  ");
        }
    }
    printf("\n");
}

#if 0
static void __dumppiece(bt_client_t* bt)
{
    int fd;
    fd = open("piecedump", O_CREAT | O_WRONLY);
    for (ii = 0; ii < bt_piecedb_get_length(bt->db); ii++)
    {
        bt_piece_t *pce;
        
        pce = bt_piecedb_get(bt->db, ii);
        write(fd, (void*)bt_piece_get_data(pce), bt_piece_get_size(pce));
    }

    close(fd);
}
#endif

