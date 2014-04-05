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

#include "bitfield.h"
#include "bt.h"
#include "bt_piece_db.h"
#include "bt_piece.h"

#include "linked_list_hashmap.h"

/* for finding empty slots */
#include "sparse_counter.h"

typedef struct
{
    /* piece map */
    hashmap_t *pmap;

    sparsecounter_t *space;

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
    priv(db)->pmap = hashmap_new(__piece_hash, __piece_cmp, 11);
    priv(db)->space = sc_init(1 << 31);
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

    return hashmap_get(priv(db)->pmap, (void*)((unsigned long)idx+1));
}

int bt_piecedb_count(bt_piecedb_t * db)
{
    return hashmap_count(priv(db)->pmap);
}

int bt_piecedb_add_with_hash_and_size(bt_piecedb_t * db,
    const unsigned char *sha1sum, const int piece_bytes_size)
{
    int i = bt_piecedb_add(db, 1);
    void *p = bt_piecedb_get(db, i);
    bt_piece_set_hash(p,sha1sum);
    bt_piece_set_size(p,piece_bytes_size);
    return i;
}

int bt_piecedb_add(bt_piecedb_t * db, unsigned int npieces)
{
    /* get space for this block of pieces */
    int idx, len;
    sc_get_incomplete(priv(db)->space, &idx, &len, npieces);
    //printf("len %d npieces %d\n", len, npieces);
    assert(len == npieces);
    return bt_piecedb_add_at_idx(db, npieces, idx);
}

int bt_piecedb_add_at_idx(bt_piecedb_t * db, unsigned int npieces, int idx)
{
    if (sc_have(priv(db)->space,idx, npieces))
        return -1;

    sc_mark_complete(priv(db)->space, idx, npieces);

    int i;
    for (i=0; i<npieces; i++)
    {
        bt_piece_t *p = bt_piece_new(NULL, 0);
        bt_piece_set_disk_blockrw(p, priv(db)->blockrw, priv(db)->blockrw_data);
        bt_piece_set_idx(p, idx + i);
        hashmap_put(priv(db)->pmap, (void*)((unsigned long)p->idx+1), p);
    }

    return idx;
}

void bt_piecedb_remove(bt_piecedb_t * db, int idx)
{
    // TODO memleak here?
    hashmap_remove(priv(db)->pmap, (void*)((unsigned long)idx+1));
    sc_mark_incomplete(priv(db)->space, idx, 1);
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
    return hashmap_count(priv(db)->pmap);
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
