
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
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <arpa/inet.h>

#include "bt.h"
#include "bt_local.h"
#include "sha1.h"
#include "raprogress.h"

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

    bt_blockrw_i *disk;
    void *disk_udata;

    bt_blockrw_i irw;
} bt_piece_private_t;

#define priv(x) ((bt_piece_private_t*)(x))

/*----------------------------------------------------------------------------*/
bt_blockrw_i *bt_piece_get_blockrw(
    bt_piece_t * pce
)
{
    return &priv(pce)->irw;
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
    bt_piece_t *pce = pceo;

    int offset, len;

    assert(pce);

    offset = blk->block_byte_offset;
    len = blk->block_len;

//    printf("writing block: %d\n", blk->piece_idx, blk->block_byte_offset);

    if (!priv(pce)->disk)
    {
        return 0;
    }

    assert(priv(pce)->disk);
    assert(priv(pce)->disk->write_block);
    assert(priv(pce)->disk_udata);

    priv(pce)->disk->write_block(priv(pce)->disk_udata, pce, blk, blkdata);
    raprogress_mark_complete(priv(pce)->progress_requested, offset, len);
    raprogress_mark_complete(priv(pce)->progress_downloaded, offset, len);

    int off, ln;

    raprogress_get_incomplete(priv(pce)->progress_requested, &off, &ln,
                              priv(pce)->piece_length);
#if 0
    printf("%d left to go: %d/%d\n",
           pce->idx,
           raprogress_get_nbytes_completed(pce->progress_downloaded),
           pce->piece_length);
#endif

    return 1;
}

void *bt_piece_read_block(
    void *pceo,
    void *caller,
    const bt_block_t * blk
)
{
    bt_piece_t *pce = pceo;

    assert(priv(pce)->disk->write_block);

    if (!priv(pce)->disk->write_block)
        return NULL;

    if (!raprogress_have
        (priv(pce)->progress_downloaded, blk->block_byte_offset,
         blk->block_len))
        return NULL;


    return priv(pce)->disk->read_block(priv(pce)->disk_udata, pce, blk);
}

/*----------------------------------------------------------------------------*/

bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
)
{
    bt_piece_private_t *pce;

    pce = malloc(sizeof(bt_piece_private_t));
    priv(pce)->sha1 = malloc(20);
    memcpy(priv(pce)->sha1, sha1sum, 20);
    priv(pce)->progress_downloaded = raprogress_init(piece_bytes_size);
    priv(pce)->progress_requested = raprogress_init(piece_bytes_size);
    priv(pce)->piece_length = piece_bytes_size;
    priv(pce)->irw.read_block = bt_piece_read_block;
    priv(pce)->irw.write_block = bt_piece_write_block;
    priv(pce)->is_completed = FALSE;
//    priv(pce)->data = malloc(priv(pce)->piece_length);
//    memset(priv(pce)->data, 0, piece_bytes_size);

    assert(priv(pce)->progress_downloaded);
    assert(priv(pce)->progress_requested);
    return (bt_piece_t *) pce;
}

void bt_piece_free(
    bt_piece_t * pce
)
{
    free(priv(pce)->sha1);
    raprogress_free(priv(pce)->progress_downloaded);
    raprogress_free(priv(pce)->progress_requested);
    free(pce);
}

/* 
 ** get data via block read */
static void *__get_data(
    bt_piece_t * pce
)
{
    bt_block_t tmp;

    /*  fail without disk writing functions */
    if (!priv(pce)->disk || !priv(pce)->disk->read_block)
    {
        assert(FALSE);
    }

    /*  read this block */
    tmp.piece_idx = priv(pce)->idx;
    tmp.block_byte_offset = 0;
    tmp.block_len = priv(pce)->piece_length;
    /*  go to the disk */
    return priv(pce)->disk->read_block(priv(pce)->disk_udata, pce, &tmp);
}

/*----------------------------------------------------------------------------*/

void bt_piece_set_disk_blockrw(
    bt_piece_t * pce,
    bt_blockrw_i * irw,
    void *udata
)
{
//    assert(irw->write_block);
//    assert(irw->read_block);
//    assert(udata);

    priv(pce)->disk = irw;
    priv(pce)->disk_udata = udata;
}

void *bt_piece_get_data(
    bt_piece_t * pce
)
{
    return __get_data(pce);
}

/*----------------------------------------------------------------------------*/

bool bt_piece_is_valid(
    bt_piece_t * pce
)
{
//    printf("our hash: %.*s\n", 20, pce->sha1);

    unsigned char *sha1;

    void *data;

    if (!(data = __get_data(pce)))
    {
        return 0;
    }

//    printf("piece_length: %d\n", priv(pce)->piece_length);
#if 0
    SHA1Context sha;

    SHA1Reset(&sha);
    SHA1Input(&sha, data, priv(pce)->piece_length);
    SHA1Result(&sha);

    /*  convert to big endian */
    sha1 = (unsigned char *) sha.Message_Digest;
    for (ii = 0; ii < 5; ii++)
        sha.Message_Digest[ii] = htonl(sha.Message_Digest[ii]);
#endif

#if 0
    SHA1_CTX ctx;

    int ii;

    unsigned char hash[20];

    SHA1Init(&ctx);
    SHA1Update(&ctx, data, priv(pce)->piece_length);
    SHA1Final(hash, &ctx);
//    for(i=0;i<20;i++)
//        printf("%02x", hash[i]);

#endif

#if 0
    printf("created: ");
    for (ii = 0; ii < 20; ii++)
        printf("%2.0x ", sha1[ii]);
    printf("\n");
    printf("expected:");
    for (ii = 0; ii < 20; ii++)
        printf("%2.0x ", (unsigned char) priv(pce)->sha1[ii]);
    printf("\n");
#endif

    char *hash;

    int ret;

    hash = str2sha1hash(data, priv(pce)->piece_length);
    ret = bt_sha1_equal(hash, priv(pce)->sha1);
    free(hash);
    return ret;
//    return bt_sha1_equal(sha1, priv(pce)->sha1);
}

bool bt_piece_is_complete(
    bt_piece_t * pce
)
{
    if (priv(pce)->is_completed)
    {
        return TRUE;
    }
#if 1
    else if (raprogress_is_complete(priv(pce)->progress_downloaded))
    {
        return bt_piece_is_valid(pce);
    }
#endif
    else
    {
        int off, ln;

        raprogress_get_incomplete(priv(pce)->progress_downloaded, &off, &ln,
                                  priv(pce)->piece_length);

        /*  if we haven't downloaded any of the file */
        if (0 == off && ln == priv(pce)->piece_length)
        {
            /* if sha1 matches properly - then we are done */
            if (bt_piece_is_valid(pce))
            {
                priv(pce)->is_completed = TRUE;
                return TRUE;
            }
        }

        return FALSE;
    }
}

bool bt_piece_is_fully_requested(
    bt_piece_t * pce
)
{
    return raprogress_is_complete(priv(pce)->progress_requested);
}

/*----------------------------------------------------------------------------*/

/*
 * Build the following request block for the peer, from this piece.
 * Assume that we want to complete the piece by going through the piece in 
 * sequential blocks. */
void bt_piece_poll_block_request(
    bt_piece_t * pce,
    bt_block_t * request
)
{
//    assert(!bt_piece_is_done(pce));

    int offset, len, block_size;

    /*  very rare that the standard block size is greater than the piece size
     *  this should relate to testing only */
    if (priv(pce)->piece_length < BLOCK_SIZE)
    {
        block_size = priv(pce)->piece_length;
//        printf("blocksize: %d\n", block_size);
    }
    else
    {
        block_size = BLOCK_SIZE;
    }

    /*  get */
    raprogress_get_incomplete(priv(pce)->progress_requested, &offset, &len,
                              block_size);

    request->piece_idx = priv(pce)->idx;
    request->block_byte_offset = offset;
    request->block_len = len;
//    printf("polled: zpceidx: %d block_byte_offset: %d block_len: %d\n",
//           request->piece_idx, request->block_byte_offset, request->block_len);
    /*  mark */
    raprogress_mark_complete(priv(pce)->progress_requested, offset, len);
}

/*----------------------------------------------------------------------------*/

void bt_piece_set_idx(
    bt_piece_t * pce,
    const int idx
)
{
    priv(pce)->idx = idx;
}

int bt_piece_get_idx(
    bt_piece_t * pce
)
{
    return pce->idx;
}

char *bt_piece_get_hash(
    bt_piece_t * pce
)
{
    return priv(pce)->sha1;
}

int bt_piece_get_size(
    bt_piece_t * pce
)
{
    return priv(pce)->piece_length;
}

/*----------------------------------------------------------------------------*/
#if 0
void bt_piece_do_progress(
    bt_piece_t * pce,
    const int offset,
    const int len
)
{
}
#endif

#if 0
int bt_piece_block_is_completed(
    bt_piece_t * pce,
    const int offset,
    const int len
)
{
    return raprogress_get_incomplete(pce->progress_requested,
                                     &off, &ln, pce->piece_length);
}
#endif

void bt_piece_write_block_to_stream(
    bt_piece_t * pce,
    bt_block_t * blk,
    byte ** msg
)
{
    byte *data;

    int ii;

    data = __get_data(pce) + blk->block_byte_offset;

    for (ii = 0; ii < blk->block_len; ii++)
    {
        byte val;

        val = *(data + ii);
        stream_write_ubyte(msg, val);
    }
}

int bt_piece_write_block_to_str(
    bt_piece_t * pce,
    bt_block_t * blk,
    char *out
)
{
    int offset, len;

    void *data = __get_data(pce);

    offset = blk->block_byte_offset;
    len = blk->block_len;
    memcpy(out, (byte *) data + offset, len);
#if 0
    for (ii = 0; ii < len; ii++)
    {
        byte val;

        val = *((byte *) priv(pce)->data + offset + ii);
        stream_write_ubyte(*msg, val);
    }
#endif

    return 1;
}
