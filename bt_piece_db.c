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

    /* default size of piece */
    int pce_len_bytes;
    int tot_file_size_bytes;

    /*  reader and writer of blocks to disk */
    bt_blockrw_i *blockrw;
    void *blockrw_data;

} bt_piecedb_private_t;

#define priv(x) ((bt_piecedb_private_t*)(x))

static unsigned long __piece_hash(const void *obj)
{
    //const bt_piece_t* p = obj;
    //return p->idx;
    //const unsigned int* p = (void*)obj;
    const unsigned long p = (unsigned long)obj;
    return p;
}

static long __piece_cmp(const void *obj, const void *other)
{
//    const bt_piece_t* p1 = obj;
//    const bt_piece_t* p2 = other;
//    return p1->idx - p2->idx;
    const unsigned long p1 = (unsigned long)obj;
    const unsigned long p2 = (unsigned long)other;
    return p2 - p1;
}

bt_piecedb_t *bt_piecedb_new()
{
    bt_piecedb_t *db;

    db = calloc(1, sizeof(bt_piecedb_private_t));
    priv(db)->tot_file_size_bytes = 0;
    //priv(db)->pieces_size = 999;
    //priv(db)->pieces = malloc(sizeof(bt_piece_t *) * priv(db)->pieces_size);
    priv(db)->pieces = hashmap_new(__piece_hash, __piece_cmp, 11);
    return db;
}

#if 0
int bt_piecedb_get_length(bt_piecedb_t * db)
{
    return priv(db)->tot_file_size_bytes;
}
#endif

void bt_piecedb_set_piece_length(bt_piecedb_t * db, const int pce_len_bytes)
{
    priv(db)->pce_len_bytes = pce_len_bytes;
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

/**
 * Obtain this piece from the piece database
 * @return piece specified by piece_idx; otherwise NULL
 */
void *bt_piecedb_get(void* dbo, const unsigned int idx)
{
    bt_piecedb_t * db = dbo;

    return hashmap_get(priv(db)->pieces, (void*)((unsigned long)idx+1));
}

#if 0
/** 
 * Take care of the situation where the last piece is sized differently */
static int __figure_out_new_piece_size(bt_piecedb_t * db)
{
    int tot_bytes_used, ii;

    /* figure out current total size */
    for (ii = 0, tot_bytes_used = 0;
            ii < hashmap_count(priv(db)->pieces); ii++)
    {
        bt_piece_t *p;
        
        p = bt_piecedb_get(db,ii);
        tot_bytes_used += bt_piece_get_size(p);
    }

    int tot_size = bt_piecedb_get_tot_file_size(db);
    assert(tot_bytes_used <= tot_size);
    if (tot_size - tot_bytes_used < priv(db)->pce_len_bytes)
    {
        return tot_size - tot_bytes_used;
    }
    else
    {
        return priv(db)->pce_len_bytes;
    }
}
#endif

/**
 * @return number of pieces */
int bt_piecedb_count(bt_piecedb_t * db)
{
    return hashmap_count(priv(db)->pieces);
}

/**
 * Add a piece with this sha1sum
 * @param length Piece's size in bytes
 * @return piece idx, otherwise -1 on error */
int bt_piecedb_add(bt_piecedb_t * db, const char *sha1, unsigned int size)
{
    bt_piece_t *pce;

#if 0
    int size;

    /* check if we have enough file space for this piece */
    if (0 == (size = __figure_out_new_piece_size(db)))
    {
        printf("done\n");
        return -1;
    }
#endif

#if 0 /* debugging */
    printf("adding piece: %d bytes %d piecelen:%d\n",
            size, priv(db)->tot_file_size_bytes, priv(db)->pce_len_bytes);
    {
        int ii;

        for (ii=0; ii<20; ii++)
            printf("%02x ", ((unsigned char*)sha1)[ii]);
        printf("\n");
    }
#endif

    assert(priv(db)->pieces);

    /* create piece */
    pce = bt_piece_new((const unsigned char*)sha1, size);
    bt_piece_set_idx(pce, hashmap_count(priv(db)->pieces));
    bt_piece_set_disk_blockrw(pce, priv(db)->blockrw, priv(db)->blockrw_data);
    hashmap_put(priv(db)->pieces, (void*)((unsigned long)pce->idx+1), pce);
    return pce->idx;
}

/**
 * Remove a piece with this idx */
void bt_piecedb_remove(bt_piecedb_t * db, int idx)
{
    // TODO memleak here?
    hashmap_remove(priv(db)->pieces, (void*)((unsigned long)idx+1));
}

/**
 * Mass add pieces to the piece database
 *
 * @param pieces A string of 20 byte sha1 hashes. Is always a multiple of 20 bytes in length. 
 * @param len: total length of combine hash string
 * */
#if 0
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
#endif

/**
 * @return number of pieces downloaded */
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

/**
 * @return number of pieces downloaded */
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

/**
 * @return 1 if all complete, 0 otherwise */
int bt_piecedb_get_length(bt_piecedb_t * db)
{
    return hashmap_count(priv(db)->pieces);
}

/**
 * @return 1 if all complete, 0 otherwise */
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

/**
 * Increase total file size by this file's size */
void bt_piecedb_increase_piece_space(bt_piecedb_t* db, const int size)
{
    bt_piecedb_set_tot_file_size(db,
            bt_piecedb_get_tot_file_size(db) + size);
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
