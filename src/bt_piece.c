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

#include "bt_block.h"
#include "bt.h"
#include "bt_local.h"

/* for bt_piece_write_block return codes */
#include "bt_piece.h"

#include "bt_sha1.h"
#include "sparse_counter.h"
#include "avl_tree.h"

enum {
    VALIDITY_NOTCHECKED,
    VALIDITY_VALID,
    VALIDITY_INVALID
};

typedef struct
{
    /* idx must align with bt_piece_t */
    int idx;

    avltree_t* peers;

    int validity;

    int piece_length;

    /* downloaded: we have this block downloaded */
    sparsecounter_t *progress_downloaded;

    /* we have requested this block */
    sparsecounter_t *progress_requested;

    char *sha1;

    /* if we calculate that we are completed, cache this result */
    int is_completed;

    /* functions and data for reading/writing block data */
    bt_blockrw_i *disk;
    void *disk_udata;
} __piece_private_t;

#define priv(x) ((__piece_private_t*)(x))

void* bt_piece_get_peers(bt_piece_t *me, int *iter)
{
    for (;*iter < avltree_size(priv(me)->peers); (*iter)++)
    {
        void* k;

        if ((k = avltree_get_from_idx(priv(me)->peers,*iter)))
        {
            (*iter)++;
            return k;
        }
    }

    return NULL;
}

int bt_piece_num_peers(bt_piece_t *me)
{
    return avltree_count(priv(me)->peers);
}

int bt_piece_write_block(
    bt_piece_t *me,
    void *caller,
    const bt_block_t * b,
    const void *b_data,
    void* peer
)
{
    assert(me);

#if 0 /*  debugging */
    printf("writing block: %d\n", b->piece_idx, b->offset);
#endif

    if (!priv(me)->disk)
    {
        return 0;
    }

    avltree_insert(priv(me)->peers, peer, peer);

    assert(priv(me)->disk->write_block);
    assert(priv(me)->disk_udata);

    if (0 == priv(me)->disk->write_block(priv(me)->disk_udata, me, b, b_data))
    {
        return 0;
    }
    else
    {
        priv(me)->validity = VALIDITY_NOTCHECKED;
    }

    /* mark progress */
    sc_mark_complete(priv(me)->progress_requested, b->offset, b->len);
    sc_mark_complete(priv(me)->progress_downloaded, b->offset, b->len);

#if 0 /*  debugging */
    printf("%d left to go: %d/%d\n",
           me->idx,
           sc_get_nbytes_completed(priv(me)->progress_downloaded),
           priv(me)->piece_length);
#endif

    if (sc_is_complete(priv(me)->progress_downloaded))
        return BT_PIECE_WRITE_BLOCK_COMPLETELY_DOWNLOADED;

    return BT_PIECE_WRITE_BLOCK_SUCCESS;
}

void *bt_piece_read_block(bt_piece_t *me, void *caller, const bt_block_t * b)
{
    assert(priv(me)->disk->read_block);

    if (!priv(me)->disk->read_block)
        return NULL;

    if (!sc_have(priv(me)->progress_downloaded, b->offset, b->len))
        return NULL;

    return priv(me)->disk->read_block(priv(me)->disk_udata, me, b);
}

static long __cmp_address(
    const void *e1,
    const void *e2
)
{
    return (unsigned long)e2 - (unsigned long)e1;
}

bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
)
{
    __piece_private_t *me;

    me = calloc(1,sizeof(__piece_private_t));
    priv(me)->progress_downloaded = sc_init(piece_bytes_size);
    priv(me)->progress_requested = sc_init(piece_bytes_size);
    priv(me)->piece_length = piece_bytes_size;
    priv(me)->is_completed = FALSE;
    priv(me)->peers = avltree_new(__cmp_address);
    priv(me)->sha1 = malloc(20);
    memcpy(priv(me)->sha1, sha1sum, 20);
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

/** 
 * Get data via block read */
static void *__get_data(
    bt_piece_t * me
)
{
    bt_block_t tmp;

    /* fail without disk reading functions */
    if (!priv(me)->disk || !priv(me)->disk->read_block)
    {
#if 0 /* debugging */
        printf("ERROR: no disk reading functions available\n");
#endif
        return NULL;
    }

    /* read whole piece */
    tmp.piece_idx = me->idx;
    tmp.offset = 0;
    tmp.len = priv(me)->piece_length;

#if 0 /*  debugging */
    printf("loading: %d %d %d\n", tmp.piece_idx, tmp.offset, tmp.len);
#endif

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

void *bt_piece_get_data(bt_piece_t * me)
{
    return __get_data(me);
}

int bt_piece_is_valid(bt_piece_t * me)
{
    switch (priv(me)->validity)
    {
    case VALIDITY_VALID: return 1;
    case VALIDITY_INVALID: return 0;
    case VALIDITY_NOTCHECKED: return -1;
    default: assert(0); break;
    }
}

int bt_piece_is_downloaded(bt_piece_t * me)
{
    return sc_is_complete(priv(me)->progress_downloaded);
}

int bt_piece_is_complete(bt_piece_t * me)
{
    if (priv(me)->is_completed)
    {
        return TRUE;
    }

    int off, ln;

    sc_get_incomplete(priv(me)->progress_downloaded, &off, &ln,
                              priv(me)->piece_length);

    /*  if we haven't downloaded any of the file */
    if (0 == off && ln == priv(me)->piece_length)
    {
        if (1 == bt_piece_is_valid(me))
        {
            priv(me)->is_completed = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}

int bt_piece_is_fully_requested(bt_piece_t * me)
{
    return sc_is_complete(priv(me)->progress_requested);
}

void bt_piece_poll_block_request(bt_piece_t * me, bt_block_t * request)
{
    int offset, len, blk_size;

    /*  very rare that the standard block size is greater than the piece size
     *  this should relate to testing only */
    if (priv(me)->piece_length < BLOCK_SIZE)
    {
        blk_size = priv(me)->piece_length;
    }
    else
    {
        blk_size = BLOCK_SIZE;
    }

    /* create the request by getting an incomplete block */
    sc_get_incomplete(priv(me)->progress_requested, &offset, &len, blk_size);
    request->piece_idx = me->idx;
    request->offset = offset;
    request->len = len;

#if 0 /* debugging */
    printf("polled: pceidx: %d offset: %d len: %d\n",
           request->piece_idx, request->offset, request->len);
#endif

    /* mark requested counter */
    sc_mark_complete(priv(me)->progress_requested, offset, len);
}

void bt_piece_giveback_block(bt_piece_t * me, bt_block_t * b)
{
    sc_mark_incomplete(priv(me)->progress_requested, b->offset, b->len);
}

void bt_piece_set_complete(bt_piece_t * me, int yes)
{
    priv(me)->is_completed = yes;
}

void bt_piece_set_idx(bt_piece_t * me, const int idx)
{
    me->idx = idx;
}

int bt_piece_get_idx(bt_piece_t * me)
{
    return me->idx;
}

char *bt_piece_get_hash(bt_piece_t * me)
{
    return priv(me)->sha1;
}

int bt_piece_get_size(bt_piece_t * me)
{
    assert(me);
    return priv(me)->piece_length;
}

int bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    unsigned char ** msg
)
{
    unsigned char *data;
    int ii;

    if (!(data = __get_data(me)))
        return 0;

    data += blk->offset;

    for (ii = 0; ii < blk->len; ii++)
    {
        unsigned char val = *(data + ii);
        bitstream_write_ubyte(msg, val);
    }

    return 1;
}

int bt_piece_write_block_to_str(bt_piece_t * me, bt_block_t * blk, char *out)
{
    int offset, len;
    void *data;
    
    data = __get_data(me);
    offset = blk->offset;
    len = blk->len;
    memcpy(out, (unsigned char *) data + offset, len);
    return 1;
}

void bt_piece_drop_download_progress(bt_piece_t *me)
{
    avltree_empty(priv(me)->peers);
    priv(me)->is_completed = 0;
    priv(me)->validity = VALIDITY_NOTCHECKED;
    sc_mark_all_incomplete(priv(me)->progress_downloaded);
    sc_mark_all_incomplete(priv(me)->progress_requested);
}

int bt_piece_validate(bt_piece_t* me)
{
#if 0
    if (!sc_is_complete(priv(me)->progress_downloaded))
    {
        return 0;
    }
#endif

    void *data;

    if (!(data = __get_data(me)))
    {
        return 0;
    }

    char hash[21];
    int ret;

    bt_str2sha1hash(hash, data, priv(me)->piece_length);
    ret = bt_sha1_equal(hash, priv(me)->sha1);

    if (1 == ret)
    {
        priv(me)->validity = VALIDITY_VALID;
        priv(me)->is_completed = TRUE;
    }
    else
    {
        priv(me)->validity = VALIDITY_INVALID;
        priv(me)->is_completed = FALSE;
        return 0;
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
    return ret;
}

