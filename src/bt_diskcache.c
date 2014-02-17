/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Manages layer between memory and disk
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bt.h"
#include "bt_local.h"

#include "pseudolru.h"

typedef struct
{
    unsigned char *data;
    int idx;
} mpiece_t;

typedef struct
{
    bt_blockrw_i irw;

    mpiece_t **pieces;
    int npieces;

    int piece_length;

    bt_blockrw_i *disk;
    void *disk_udata;

    /*  least recently used piece */
    pseudolru_t *lru_piece;

    /* logger */
    func_log_f func_log;
    void *logger_data;
} diskcache_private_t;

#define priv(x) ((diskcache_private_t*)(x))

static int __lru_piece_compare(
    const void *e1,
    const void *e2
)
{
    return e1 - e2;
}

static void __log(
    bt_diskcache_t * me,
    const char *format,
    ...
)
{
    char buf[1000];

    va_list args;

    if (!priv(me)->func_log)
        return;

    va_start(args, format);
    vsprintf(buf, format, args);
    priv(me)->func_log(priv(me)->logger_data, me, buf);
}

/**
 * Get piece as per this piece_idx
 * 
 * @return always return a mpiece for us to read/write memory from/to
 */
static mpiece_t *__get_piece(
    bt_diskcache_t * me,
    const int idx
)
{
    /*  allocate list space if we don't have enough slots */
    if (priv(me)->npieces <= idx)
    {
        int ii;

        ii = priv(me)->npieces;
        priv(me)->npieces = idx + 1;
        priv(me)->pieces =
            realloc(priv(me)->pieces, sizeof(mpiece_t **) * priv(me)->npieces);

        for (; ii < priv(me)->npieces; ii++)
        {
            priv(me)->pieces[ii] = NULL;
        }
    }

    /*  if memory isn't allocated yet... */
    if (!priv(me)->pieces[idx])
    {
        priv(me)->pieces[idx] = calloc(1, sizeof(mpiece_t));
        priv(me)->pieces[idx]->idx = idx;
    }

    return priv(me)->pieces[idx];
}

void bt_diskcache_set_func_log(
    bt_diskcache_t * me,
    func_log_f log,
    void *udata
)
{
    priv(me)->func_log = log;
    priv(me)->logger_data = udata;
}

/**
 * Dump this piece_idx to the disk */
static void __diskdump_piece(
    bt_diskcache_t * me,
    const int piece_idx
)
{
    bt_block_t blk;
    mpiece_t *mpce;

    mpce = __get_piece(me, piece_idx);
    blk.piece_idx = piece_idx;
    blk.offset = 0;
    blk.len = priv(me)->piece_length;
    if (0 == priv(me)->disk->write_block(priv(me)->disk_udata, me, &blk, mpce->data))
    {

    }

    pseudolru_remove(priv(me)->lru_piece, (void *) mpce);
    free(mpce->data);
    mpce->data = NULL;
}

/**
 * @return 0 on error */
static int __write_block(
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
)
{
    bt_diskcache_t *me = udata;
    mpiece_t *mpce;

    mpce = __get_piece(me, blk->piece_idx);

    /*  malloc memory if we haven't */
    if (!mpce->data)
    {
        mpce->data = calloc(1, priv(me)->piece_length);
        assert(mpce->idx == blk->piece_idx);
    }

#if 0 /* debugging */
    printf("me-writeblock: %d %d %d %d\n",
           blk->piece_idx, blk->offset, blk->len,
           pseudolru_count(priv(me)->lru_piece));
#endif

    assert(0 < priv(me)->piece_length);
    assert(mpce->data);

    /* TODO: remove memcpy for zero-copy */
    memcpy(mpce->data + blk->offset, blkdata, blk->len);

    /*  touch piece to show how recent it is */
    pseudolru_put(priv(me)->lru_piece, (void *) mpce, (void *) mpce);

#if 0
    /* check if we have enough pieces to write out to disk */
    if (20 < pseudolru_count(priv(me)->lru_piece))
    {
        int ii;

        /*  write out ten to disk */

        for (ii=0; ii < 1; ii++)
        {
            mpce = pseudolru_pop_lru(priv(me)->lru_piece);
            __diskdump_piece(me, mpce->idx);
        }
    }
#endif

    return 1;
}

static int __flush_block(
    void *udata,
    void *caller,
    const bt_block_t * blk
)
{
    bt_diskcache_t *me = udata;
    mpiece_t *mpce;

    mpce = __get_piece(me, blk->piece_idx);

    if (!mpce->data)
    {
        return 0;
    }

    __diskdump_piece(me,blk->piece_idx);
    //__write_block(udata,caller,blk,blkdata);

    return 1;
}

static void *__get_piece_data_from_disk(
    bt_diskcache_t * me,
    const int piece_idx
)
{
    bt_block_t blk;

    blk.piece_idx = piece_idx;
    blk.offset = 0;
    blk.len = priv(me)->piece_length;
    return priv(me)->disk->read_block(priv(me)->disk_udata, me, &blk);
}

/**
 * Read data
 * Check if we have the data in the cache;
 * Otherwise retrieve the memory from the disk
 * @todo we should let the caller know if pagefaults occur
 * @return pointer to data; otherwise NULL
 * */
static void *__read_block(
    void *udata,
    void *caller,
    const bt_block_t * blk
)
{
    bt_diskcache_t *me = udata;

    mpiece_t *mpce;

#if 0 /*  debugging */
    printf("me-readblock: %d %d %d\n",
            blk->piece_idx, blk->offset, blk->len);
#endif

    mpce = __get_piece(me, blk->piece_idx);

    /* do we have the data in memory? */
    if (!mpce->data)
    {
        if (!(mpce->data = __get_piece_data_from_disk(me, blk->piece_idx)))
        {
            mpce->data = malloc(priv(me)->piece_length);
            mpce->idx = blk->piece_idx;
            memset(mpce->data, 0, priv(me)->piece_length);
        }
        else
        {
            return NULL;
        }
    }

    assert(mpce->data);

    /*  touch piece to show how recent it is */
    pseudolru_put(priv(me)->lru_piece, (void *) mpce, (void *) mpce);

    return mpce->data + blk->offset;
}

void *bt_diskcache_new(
)
{
    bt_diskcache_t *me;

    me = calloc(1, sizeof(diskcache_private_t));
    priv(me)->irw.write_block = __write_block;
    priv(me)->irw.read_block = __read_block;
    priv(me)->irw.flush_block = __flush_block;
    priv(me)->piece_length = 0;
    priv(me)->lru_piece = pseudolru_initalloc(__lru_piece_compare);
    return me;
}

void bt_diskcache_set_size(
    void *dco,
    const int piece_bytes_size
)
{
    bt_diskcache_t *me = dco;

    assert(0 < piece_bytes_size);
    priv(me)->piece_length = piece_bytes_size;
}

/**
 * Set the blockrw that we want to use to write to disk */
void bt_diskcache_set_disk_blockrw(
    void *dco,
    bt_blockrw_i * irw,
    void *irw_data
)
{
    bt_diskcache_t *me = dco;

    assert(irw->write_block);
    assert(irw->read_block);
    priv(me)->disk = irw;
    priv(me)->disk_udata = irw_data;
}

bt_blockrw_i *bt_diskcache_get_blockrw(
    void *dco
)
{
    bt_diskcache_t *me = dco;

    return &priv(me)->irw;
}

void bt_diskcache_set_piece_length(void* dco, int piece_length)
{
    bt_diskcache_t *me = dco;

    priv(me)->piece_length = piece_length;
}

/**
 * put all pieces onto disk */
void bt_diskcache_disk_dump(
    void *dco
)
{
    bt_diskcache_t *me = dco;

    int ii;

    for (ii = 0; ii < priv(me)->npieces; ii++)
    {
        mpiece_t *pce;

        pce = __get_piece(me, ii);

        if (pce->data)
        {
            __diskdump_piece(me, ii);
        }
    }
}
