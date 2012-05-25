/**
 * @file
 * @brief Manages layer between memory and disk
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
#include <stdarg.h>
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

#include "pseudolru.h"

typedef struct
{
    byte *data;
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

    func_log_f func_log;
    void *logger_data;

    /*  the number of pieces in memory */
//    int npieces_in_mem;

    /*  least recently used piece */
    pseudolru_t *lru_piece;
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
    bt_diskcache_t * dc,
    const char *format,
    ...
)
{
    char buf[1000];

    va_list args;

    if (!priv(dc)->func_log)
        return;

    va_start(args, format);
    vsprintf(buf, format, args);
    priv(dc)->func_log(priv(dc)->logger_data, dc, buf);
}

/*
 * Get piece as per this piece_idx
 * 
 * @return always return a mpiece for us to read/write memory from/to
 */
static mpiece_t *__get_piece(
    bt_diskcache_t * dc,
    const int idx
)
{
    /*  allocate list space if we don't have enough slots */
    if (priv(dc)->npieces <= idx)
    {
        int ii;

        ii = priv(dc)->npieces;
        priv(dc)->npieces = idx + 1;
        priv(dc)->pieces =
            realloc(priv(dc)->pieces, sizeof(mpiece_t **) * priv(dc)->npieces);

        for (; ii < priv(dc)->npieces; ii++)
        {
            priv(dc)->pieces[ii] = NULL;
        }
    }

    /*  if memory isn't allocated yet... */
    if (!priv(dc)->pieces[idx])
    {
        priv(dc)->pieces[idx] = calloc(1, sizeof(mpiece_t));
    }

    return priv(dc)->pieces[idx];
}

void bt_diskcache_set_func_log(
    bt_diskcache_t * dc,
    func_log_f log,
    void *udata
)
{
    priv(dc)->func_log = log;
    priv(dc)->logger_data = udata;
}

/*----------------------------------------------------------------------------*/

/**
 * Dump this piece_idx to the disk */
static void __diskdump_piece(
    bt_diskcache_t * dc,
    const int piece_idx
)
{
    bt_block_t blk;
    mpiece_t *mpce;

    mpce = __get_piece(dc, piece_idx);

    blk.piece_idx = piece_idx;
    blk.block_byte_offset = 0;
    blk.block_len = priv(dc)->piece_length;
    priv(dc)->disk->write_block(priv(dc)->disk_udata, dc, &blk, mpce->data);

    pseudolru_remove(priv(dc)->lru_piece, (void *) mpce);
    free(mpce->data);
    mpce->data = NULL;
//    priv(dc)->npieces_in_mem -= 1;
//            __write_block(dc, NULL, &blk, mpce->data);
}

/*----------------------------------------------------------------------------*/
static int __write_block(
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
)
{
    bt_diskcache_t *dc = udata;
    mpiece_t *mpce;

    mpce = __get_piece(dc, blk->piece_idx);

    /*  malloc memory if we haven't */
    if (!mpce->data)
    {
//        priv(dc)->npieces_in_mem += 1;
        mpce->data = calloc(1, priv(dc)->piece_length);
        mpce->idx = blk->piece_idx;
    }

//    printf("dc-writeblock: %d %d %d\n",
//           blk->piece_idx, blk->block_byte_offset, blk->block_len);

    assert(0 < priv(dc)->piece_length);
    assert(mpce->data);
    memcpy(mpce->data + blk->block_byte_offset, blkdata, blk->block_len);

    /*  touch piece to show how recent it is */
    pseudolru_put(priv(dc)->lru_piece, (void *) mpce, (void *) mpce);

    if (20 < pseudolru_count(priv(dc)->lru_piece))
    {
        int ii;

        /*  write out ten to disk */

        for (ii = 0; ii < 10; ii++)
        {
            mpce = pseudolru_pop_lru(priv(dc)->lru_piece);
            __diskdump_piece(dc, mpce->idx);
        }
    }

    return 1;
}

static void *__get_piece_data_from_disk(
    bt_diskcache_t * dc,
    const int piece_idx
)
{
    bt_block_t blk;

    blk.piece_idx = piece_idx;
    blk.block_byte_offset = 0;
    blk.block_len = priv(dc)->piece_length;
//    printf("piece length: %d\n", priv(dc)->piece_length);
    return priv(dc)->disk->read_block(priv(dc)->disk_udata, dc, &blk);
}

/**
 * read data.
 * Check if we have the data in the cache;
 * Otherwise retrieve the memory from the disk */
static void *__read_block(
    void *udata,
    void *caller,
    const bt_block_t * blk
)
{
    bt_diskcache_t *dc = udata;

    mpiece_t *mpce;

#if 0
    printf("dc-readblock: %d %d %d\n", blk->piece_idx,
           blk->block_byte_offset, blk->block_len);
#endif

    mpce = __get_piece(dc, blk->piece_idx);

    /*  bring in data from the hard drive */
    if (!mpce->data)
    {
//        priv(dc)->npieces_in_mem += 1;
        mpce->data = __get_piece_data_from_disk(dc, blk->piece_idx);

        if (!mpce->data)
        {
            mpce->data = malloc(priv(dc)->piece_length);
            memset(mpce->data, 0, priv(dc)->piece_length);
        }
    }

    assert(mpce->data);

    /*  touch piece to show how recent it is */
    pseudolru_put(priv(dc)->lru_piece, (void *) mpce, (void *) mpce);

    return mpce->data + blk->block_byte_offset;
}

/*----------------------------------------------------------------------------*/
void *bt_diskcache_new(
)
{
    bt_diskcache_t *dc;

    dc = calloc(1, sizeof(diskcache_private_t));
    priv(dc)->irw.write_block = __write_block;
    priv(dc)->irw.read_block = __read_block;
    priv(dc)->piece_length = 0;
//    priv(dc)->npieces_in_mem = 0;
//    priv(dc)->irw.giveup_block = NULL;
    priv(dc)->lru_piece = pseudolru_initalloc(__lru_piece_compare);
    return dc;
}

/*----------------------------------------------------------------------------*/
void bt_diskcache_set_size(
    void *dco,
    const int piece_bytes_size
)
{
    bt_diskcache_t *dc = dco;

    assert(0 < piece_bytes_size);
    priv(dc)->piece_length = piece_bytes_size;
}

void bt_diskcache_set_disk_blockrw(
    void *dco,
    bt_blockrw_i * irw,
    void *irw_data
)
{
    bt_diskcache_t *dc = dco;

    assert(irw->write_block);
    assert(irw->read_block);
    priv(dc)->disk = irw;
    priv(dc)->disk_udata = irw_data;
}

/*----------------------------------------------------------------------------*/
bt_blockrw_i *bt_diskcache_get_blockrw(
    void *dco
)
{
    bt_diskcache_t *dc = dco;

    return &priv(dc)->irw;
}

/*----------------------------------------------------------------------------*/

/**
 * put all pieces onto disk */
void bt_diskcache_disk_dump(
    void *dco
)
{
    bt_diskcache_t *dc = dco;

    int ii;

    printf("dumping to disk\n");

    for (ii = 0; ii < priv(dc)->npieces; ii++)
    {
        mpiece_t *pce;

        pce = __get_piece(dc, ii);

        if (pce->data)
        {
            __diskdump_piece(dc, ii);
        }
    }
}
