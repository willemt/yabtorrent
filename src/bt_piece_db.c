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

#include "bt_block.h"

#include "bitfield.h"
#include "bt.h"
#include "bt_piece_db.h"
#include "bt_piece.h"
#include "bt_local.h"

#include "linked_list_hashmap.h"

typedef struct
{
    hashmap_t *pieces;

    int tot_file_size_bytes;

    /*  reader and writer of blocks to disk */
    bt_blockrw_i *blockrw;
    void *blockrw_data;

} bt_piecedb_private_t;

#define priv(x) ((bt_piecedb_private_t*)(x))

static unsigned long __piece_hash(const void *obj)
{
    const unsigned long p = (unsigned long)obj;
    return p;
}

static long __piece_cmp(const void *obj, const void *other)
{
    const unsigned long p1 = (unsigned long)obj;
    const unsigned long p2 = (unsigned long)other;
    return p2 - p1;
}

bt_piecedb_t *bt_piecedb_new()
{
    bt_piecedb_t *db;

    db = calloc(1, sizeof(bt_piecedb_private_t));
    priv(db)->tot_file_size_bytes = 0;
    priv(db)->pieces = hashmap_new(__piece_hash, __piece_cmp, 11);
    return db;
}

void bt_piecedb_set_tot_file_size(bt_piecedb_t * db,
                                  const int tot_file_size_bytes)
{
    priv(db)->tot_file_size_bytes = tot_file_size_bytes;
}

int bt_piecedb_get_tot_file_size(bt_piecedb_t * db)
{
    return priv(db)->tot_file_size_bytes;
}

void bt_piecedb_set_diskstorage(bt_piecedb_t * db,
                                bt_blockrw_i * irw,
                                void *udata)
{
    assert(irw->write_block);
    assert(irw->read_block);
    assert(udata);

    priv(db)->blockrw = irw;
    priv(db)->blockrw_data = udata;
}

void* bt_piecedb_get_diskstorage(bt_piecedb_t * db)
{
    assert(db);
    return priv(db)->blockrw_data;
}

void *bt_piecedb_get(void* dbo, const unsigned int idx)
{
    bt_piecedb_t * db = dbo;

    return hashmap_get(priv(db)->pieces, (void*)((unsigned long)idx+1));
}

int bt_piecedb_count(bt_piecedb_t * db)
{
    return hashmap_count(priv(db)->pieces);
}

int bt_piecedb_add(bt_piecedb_t * db, const char *sha1, unsigned int size)
{
    bt_piece_t *pce;

    assert(priv(db)->pieces);

    pce = bt_piece_new((const unsigned char*)sha1, size);
    bt_piece_set_idx(pce, hashmap_count(priv(db)->pieces));
    bt_piece_set_disk_blockrw(pce, priv(db)->blockrw, priv(db)->blockrw_data);
    hashmap_put(priv(db)->pieces, (void*)((unsigned long)pce->idx+1), pce);
    return pce->idx;
}

void bt_piecedb_remove(bt_piecedb_t * db, int idx)
{
    // TODO memleak here?
    hashmap_remove(priv(db)->pieces, (void*)((unsigned long)idx+1));
}

int bt_piecedb_get_num_downloaded(bt_piecedb_t * db)
{
    int ii, downloaded = 0;

    for (ii = 0; ii < bt_piecedb_get_length(db); ii++)
    {
        bt_piece_t *p = bt_piecedb_get(db, ii);
        if (bt_piece_is_downloaded(p))
            downloaded += 1;
    }

    return downloaded;
}

int bt_piecedb_get_num_completed(bt_piecedb_t * db)
{
    int ii, cnt = 0;

    for (ii = 0; ii < bt_piecedb_get_length(db); ii++)
    {
        bt_piece_t *p = bt_piecedb_get(db, ii);
        if (bt_piece_is_complete(p))
            cnt += 1;
    }

    return cnt;
}

int bt_piecedb_get_length(bt_piecedb_t * db)
{
    return hashmap_count(priv(db)->pieces);
}

int bt_piecedb_all_pieces_are_complete(bt_piecedb_t* db)
{
    int ii;

    for (ii = 0; ii < bt_piecedb_get_length(db); ii++)
    {
        bt_piece_t *pce = bt_piecedb_get(db, ii);
        if (!bt_piece_is_complete(pce))
            return 0;
    }

    return 1;
}

void bt_piecedb_increase_piece_space(bt_piecedb_t* db, const int size)
{
    bt_piecedb_set_tot_file_size(db,
            bt_piecedb_get_tot_file_size(db) + size);
}

void bt_piecedb_print_pieces_downloaded(bt_piecedb_t * db)
{
    int ii, depth, is_complete = 1;

    printf("pieces (%d) downloaded: \n  ", bt_piecedb_get_length(db));

    for (ii = 0, depth = 3;
         ii < bt_piecedb_get_length(db); ii++, depth += 1)
    {
        bt_piece_t * p = bt_piecedb_get(db, ii);

        if (bt_piece_is_complete(p))
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
