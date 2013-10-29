/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Represent a single piece and the accounting it needs
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
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

enum {
    VALIDITY_NOTCHECKED,
    VALIDITY_VALID,
    VALIDITY_INVALID
};

/*  bittorrent piece */
typedef struct
{
    /* index on 'bit stream' */
    int idx;

    int validity;

    int piece_length;

    /*  downloaded: we have this block downloaded */
    sparsecounter_t *progress_downloaded;

    /*  we have requested this block */
    sparsecounter_t *progress_requested;

    char *sha1;

    /*  if we calculate that we are completed, cache this result */
    int is_completed;

    /* functions and data for reading/writing block data */
    bt_blockrw_i *disk;
    void *disk_udata;

    bt_blockrw_i irw;
} __piece_private_t;

#define priv(x) ((__piece_private_t*)(x))

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

#if 0 /*  debugging */
    printf("writing block: %d\n", blk->piece_idx, blk->offset);
#endif

    if (!priv(me)->disk)
    {
        return 0;
    }

    assert(priv(me)->disk);
    assert(priv(me)->disk->write_block);
    assert(priv(me)->disk_udata);

    priv(me)->disk->write_block(priv(me)->disk_udata, me, blk, blkdata);

    priv(me)->validity = VALIDITY_NOTCHECKED;

    /* mark progress */
    {
        int offset, len;

        offset = blk->offset;
        len = blk->len;
        sc_mark_complete(priv(me)->progress_requested, offset, len);
        sc_mark_complete(priv(me)->progress_downloaded, offset, len);
//        sc_get_incomplete(priv(me)->progress_requested, &off, &ln,
//                                  priv(me)->piece_length);

        if (sc_get_nbytes_completed(priv(me)->progress_downloaded) ==
                priv(me)->piece_length)
        {
            if (bt_piece_is_complete(me))
            {
                bt_block_t p;
                p.piece_idx = blk->piece_idx;
                p.len = priv(me)->piece_length;
                p.offset = 0;
                if (priv(me)->disk->flush_block)
                    priv(me)->disk->flush_block(priv(me)->disk_udata, me, &p);
            }
        }
    }

#if 0 /*  debugging */
    printf("%d left to go: %d/%d\n",
           me->idx,
           sc_get_nbytes_completed(priv(me)->progress_downloaded),
           priv(me)->piece_length);
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

    if (!sc_have(
        priv(me)->progress_downloaded, blk->offset,
         blk->len))
        return NULL;

    return priv(me)->disk->read_block(priv(me)->disk_udata, me, blk);
}

bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
)
{
    __piece_private_t *me;

    me = calloc(1,sizeof(__piece_private_t));
    priv(me)->sha1 = malloc(20);
    memcpy(priv(me)->sha1, sha1sum, 20);
    priv(me)->progress_downloaded = sc_init(piece_bytes_size);
    priv(me)->progress_requested = sc_init(piece_bytes_size);
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
    sc_free(priv(me)->progress_downloaded);
    sc_free(priv(me)->progress_requested);
    free(me);
}

/* 
 ** get data via block read */
static void *__get_data(
    bt_piece_t * me
)
{
    bt_block_t tmp;

    /*  fail without disk reading functions */
    if (!priv(me)->disk || !priv(me)->disk->read_block)
    {
#if 0 /* debugging */
        printf("ERROR: no disk reading functions available\n");
#endif
        return NULL;
    }

    /*  read this block */
    tmp.piece_idx = priv(me)->idx;
    tmp.offset = 0;
    tmp.len = priv(me)->piece_length;

#if 0 /*  debugging */
    printf("loading: %d %d %d\n", tmp.piece_idx, tmp.offset, tmp.len);
#endif

    /*  go to the disk */
    return priv(me)->disk->read_block(priv(me)->disk_udata, me, &tmp);
}

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

    if (priv(me)->validity == VALIDITY_VALID)
    {
        return 1;
    }
    else if (priv(me)->validity == VALIDITY_INVALID)
    {
        return 0;
    }
     /* else VALIDITY_NOTCHECKED... */

    void *data;

    if (!(data = __get_data(me)))
    {
        return 0;
    }

    char *hash;
    int ret;

    hash = str2sha1hash(data, priv(me)->piece_length);
    ret = bt_sha1_equal(hash, priv(me)->sha1);

    if (1 == ret)
    {
        priv(me)->validity = VALIDITY_VALID;
    }
    else
    {
        priv(me)->validity = VALIDITY_INVALID;
    }

#if 0 /* debugging */
    {
        int ii;

        printf("(idx:%d) Expected: ", me->idx);
        for (ii=0; ii<20; ii++)
            printf("%02x ", ((unsigned char*)priv(me)->sha1)[ii]);
        printf("\n");

        printf("         File calc: ");
        for (ii=0; ii<20; ii++)
            printf("%02x ", ((unsigned char*)hash)[ii]);
        printf("\n");
    }
#endif

    free(hash);
    return ret;
}

int bt_piece_is_downloaded(bt_piece_t * me)
{
    return sc_is_complete(priv(me)->progress_downloaded);
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
    /*  check the counter if it is fully downloaded */
    else if (sc_is_complete(priv(me)->progress_downloaded))
    {
        //printf("fully complete %d\n", me->idx);
        if (1 == bt_piece_is_valid(me))
        {
            priv(me)->is_completed = TRUE;
            return  TRUE;
        }
        else
        {
            // TODO: set this to false
            //printf("invalid piece: %d\n", me->idx);
            return FALSE;
        }
    }
    else
    {
        int off, ln;

        sc_get_incomplete(priv(me)->progress_downloaded, &off, &ln,
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
    return sc_is_complete(priv(me)->progress_requested);
}

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
    sc_get_incomplete(priv(me)->progress_requested, &offset, &len,
                              block_size);

    /* create the request */
    request->piece_idx = priv(me)->idx;
    request->offset = offset;
    request->len = len;

//    printf("polled: zpceidx: %d offset: %d len: %d\n",
//           request->piece_idx, request->offset, request->len);

    /* mark requested counter */
    sc_mark_complete(priv(me)->progress_requested, offset, len);

}

void bt_piece_giveback_block(
    bt_piece_t * me,
    bt_block_t * b
)
{
//    printf("giveback: %d %d\n", b->offset, b->offset + b->len);
//    sc_print_contents(priv(me)->progress_requested);
//    printf("\n");
    sc_mark_incomplete(priv(me)->progress_requested,
            b->offset, b->len);
//    sc_print_contents(priv(me)->progress_requested);
//    printf("\n");
//    printf("requested:\n");
//    sc_print_contents(priv(me)->progress_requested);
//    printf("downloaded:\n");
//    sc_print_contents(priv(me)->progress_downloaded);
}

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

/**
 * Write the block to the byte stream */
void bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    unsigned char ** msg
)
{
    unsigned char *data;
    int ii;

    if (!(data = __get_data(me)))
        return;

    data += blk->offset;

    for (ii = 0; ii < blk->len; ii++)
    {
        unsigned char val;

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
    offset = blk->offset;
    len = blk->len;
    memcpy(out, (unsigned char *) data + offset, len);

    return 1;
}
