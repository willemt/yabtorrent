
/**
 * @file
 * @brief Represent a single piece and the accounting it needs
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
#include "sha1.h"

#include "block.h"
#include "bt.h"
#include "bt_local.h"
#include "bt_block_readwriter_i.h"
#include "sparse_counter.h"

/*  bittorrent piece */
typedef struct
{
    /* index on 'bit stream' */
    int idx;

    int piece_length;

    /*  downloaded: we have this block downloaded */
    void *progress_downloaded;

    /*  we have requested this block */
    void *progress_requested;

    char *sha1;

    /*  if we calculate that we are completed, cache this result */
    int is_completed;

    /* functions and data for reading/writing block data */
    bt_blockrw_i *disk;
    void *disk_udata;

    bt_blockrw_i irw;
} __piece_private_t;

#define priv(x) ((__piece_private_t*)(x))

/*----------------------------------------------------------------------------*/
bt_blockrw_i *bt_piece_get_blockrw(
    bt_piece_t * me
)
{
    return &priv(me)->irw;
}

/**
 ** Add this data to the piece */
int bt_piece_write_block(
    void *pceo,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
)
{
    bt_piece_t *me = pceo;

    assert(me);

//    printf("writing block: %d\n", blk->piece_idx, blk->block_byte_offset);

    if (!priv(me)->disk)
    {
        return 0;
    }

    assert(priv(me)->disk);
    assert(priv(me)->disk->write_block);
    assert(priv(me)->disk_udata);

    priv(me)->disk->write_block(priv(me)->disk_udata, me, blk, blkdata);

    /* mark progress */
    {
        int offset, len;

        offset = blk->block_byte_offset;
        len = blk->block_len;
        sparsecounter_mark_complete(priv(me)->progress_requested, offset, len);
        sparsecounter_mark_complete(priv(me)->progress_downloaded, offset, len);
//        sparsecounter_get_incomplete(priv(me)->progress_requested, &off, &ln,
//                                  priv(me)->piece_length);
    }

#if 0
    printf("%d left to go: %d/%d\n",
           me->idx,
           sparsecounter_get_nbytes_completed(me->progress_downloaded),
           me->piece_length);
#endif

    return 1;
}

void *bt_piece_read_block(
    void *pceo,
    void *caller,
    const bt_block_t * blk
)
{
    bt_piece_t *me = pceo;

    assert(priv(me)->disk->read_block);

    if (!priv(me)->disk->read_block)
        return NULL;

    if (!sparsecounter_have(
        priv(me)->progress_downloaded, blk->block_byte_offset,
         blk->block_len))
        return NULL;

    return priv(me)->disk->read_block(priv(me)->disk_udata, me, blk);
}

/*----------------------------------------------------------------------------*/

bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
)
{
    __piece_private_t *me;

    me = calloc(1,sizeof(__piece_private_t));
    priv(me)->sha1 = malloc(20);
    memcpy(priv(me)->sha1, sha1sum, 20);
    priv(me)->progress_downloaded = sparsecounter_init(piece_bytes_size);
    priv(me)->progress_requested = sparsecounter_init(piece_bytes_size);
    priv(me)->piece_length = piece_bytes_size;
    priv(me)->irw.read_block = bt_piece_read_block;
    priv(me)->irw.write_block = bt_piece_write_block;
    priv(me)->is_completed = FALSE;
//    priv(me)->data = malloc(priv(me)->piece_length);
//    memset(priv(me)->data, 0, piece_bytes_size);

    return (bt_piece_t *) me;
}

void bt_piece_free(
    bt_piece_t * me
)
{
    free(priv(me)->sha1);
    sparsecounter_free(priv(me)->progress_downloaded);
    sparsecounter_free(priv(me)->progress_requested);
    free(me);
}

/* 
 ** get data via block read */
static void *__get_data(
    bt_piece_t * me
)
{
    bt_block_t tmp;

    /*  fail without disk writing functions */
    if (!priv(me)->disk || !priv(me)->disk->read_block)
    {
        return NULL;
    }

    /*  read this block */
    tmp.piece_idx = priv(me)->idx;
    tmp.block_byte_offset = 0;
    tmp.block_len = priv(me)->piece_length;
    //printf("%d %d %d\n", tmp.piece_idx, tmp.block_byte_offset, tmp.block_len);

    /*  go to the disk */
    return priv(me)->disk->read_block(priv(me)->disk_udata, me, &tmp);
}

/*----------------------------------------------------------------------------*/

void bt_piece_set_disk_blockrw(
    bt_piece_t * me,
    bt_blockrw_i * irw,
    void *udata
)
{
    priv(me)->disk = irw;
    priv(me)->disk_udata = udata;
}

void *bt_piece_get_data(
    bt_piece_t * me
)
{
    return __get_data(me);
}

/**
 * Read data and use sha1 to determine if valid
 * @return 1 if valid; 0 otherwise */
int bt_piece_is_valid(bt_piece_t * me)
{
    char *hash;
    int ret;
    unsigned char *sha1;
    void *data;

    if (!(data = __get_data(me)))
    {
        return 0;
    }

    hash = str2sha1hash(data, priv(me)->piece_length);
    ret = bt_sha1_equal(hash, priv(me)->sha1);

#if 0 /* debugging */
    {
        int ii;

        printf("Expected: ");
        for (ii=0; ii<20; ii++)
            printf("%02x ", ((unsigned char*)priv(me)->sha1)[ii]);
        printf("\n");

        printf("File calc: ");
        for (ii=0; ii<20; ii++)
            printf("%02x ", ((unsigned char*)hash)[ii]);
        printf("\n");
    }
#endif

    free(hash);
    return ret;
}

/**
 * Determine if the piece is complete
 * A piece needs to be valid to be complete
 *
 * @return 1 if complete; 0 otherwise */
int bt_piece_is_complete(bt_piece_t * me)
{
    if (priv(me)->is_completed)
    {
        return TRUE;
    }
    else if (sparsecounter_is_complete(priv(me)->progress_downloaded))
    {
        if (1 == bt_piece_is_valid(me))
        {
            priv(me)->is_completed = TRUE;
            return  TRUE;
        }
    }
    else
    {
        int off, ln;

        sparsecounter_get_incomplete(priv(me)->progress_downloaded, &off, &ln,
                                  priv(me)->piece_length);

        /*  if we haven't downloaded any of the file */
        if (0 == off && ln == priv(me)->piece_length)
        {
            /* if sha1 matches properly - then we are done */
            if (1 == bt_piece_is_valid(me))
            {
                priv(me)->is_completed = TRUE;
                return TRUE;
            }
        }

        return FALSE;
    }

    return FALSE;
}

int bt_piece_is_fully_requested(bt_piece_t * me)
{
    return sparsecounter_is_complete(priv(me)->progress_requested);
}

/*----------------------------------------------------------------------------*/

/**
 * Build the following request block for the peer, from this piece.
 * Assume that we want to complete the piece by going through the piece in 
 * sequential blocks. */
void bt_piece_poll_block_request(
    bt_piece_t * me,
    bt_block_t * request
)
{
    int offset, len, block_size;

    /*  very rare that the standard block size is greater than the piece size
     *  this should relate to testing only */
    if (priv(me)->piece_length < BLOCK_SIZE)
    {
        block_size = priv(me)->piece_length;
    }
    else
    {
        block_size = BLOCK_SIZE;
    }

    /* get an incomplete block */
    sparsecounter_get_incomplete(priv(me)->progress_requested, &offset, &len,
                              block_size);

    /* create the request */
    request->piece_idx = priv(me)->idx;
    request->block_byte_offset = offset;
    request->block_len = len;

//    printf("polled: zpceidx: %d block_byte_offset: %d block_len: %d\n",
//           request->piece_idx, request->block_byte_offset, request->block_len);

    /* mark requested counter */
    sparsecounter_mark_complete(priv(me)->progress_requested, offset, len);
}

/*----------------------------------------------------------------------------*/

void bt_piece_set_complete(
    bt_piece_t * me,
    int yes
)
{
    priv(me)->is_completed = yes;
}

void bt_piece_set_idx(
    bt_piece_t * me,
    const int idx
)
{
    priv(me)->idx = idx;
}

int bt_piece_get_idx(
    bt_piece_t * me
)
{
    return me->idx;
}

char *bt_piece_get_hash(
    bt_piece_t * me
)
{
    return priv(me)->sha1;
}

int bt_piece_get_size(
    bt_piece_t * me
)
{
    return priv(me)->piece_length;
}

/*----------------------------------------------------------------------------*/

/**
 * Write the block to the byte stream */
void bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    byte ** msg
)
{
    byte *data;
    int ii;

    if (!(data = __get_data(me)))
        return;

    data += blk->block_byte_offset;

    for (ii = 0; ii < blk->block_len; ii++)
    {
        byte val;

        val = *(data + ii);
        bitstream_write_ubyte(msg, val);
    }
}

int bt_piece_write_block_to_str(
    bt_piece_t * me,
    bt_block_t * blk,
    char *out
)
{
    int offset, len;
    void *data;
    
    data = __get_data(me);
    offset = blk->block_byte_offset;
    len = blk->block_len;
    memcpy(out, (byte *) data + offset, len);

    return 1;
}
