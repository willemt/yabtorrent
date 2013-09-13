
/**
 * @file
 * @brief A disk layer which just works in memory
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
 *
 * @section description
 * A backend block read/writer that puts data into RAM
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

#include "block.h"
#include "bt.h"

typedef struct
{
    bt_blockrw_i irw;
    int piece_size;
    int data_size;
    unsigned char *data;
} diskmem_t;

int bt_diskmem_write_block(
    void *udata,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk,
    const void *blkdata
)
{
    diskmem_t *me = udata;
    unsigned int offset;

#if 0 /* debugging */
    int ii;

    printf("diskmem %d:", blk->offset);
    for (ii = 0; ii < blk->len; ii++)
        printf("%02x,", ((const unsigned char*)blkdata)[ii]);
    printf("\n");
//    for (ii = 0; ii < blk->len; ii++)
//        printf("%c,", ((const unsigned char*)blkdata)[ii]);
//    printf("\n");
#endif

    assert(me->data);

    offset = blk->piece_idx * me->piece_size + blk->offset;

    /* if required, enlarge capacity */
    if (me->data_size < offset + blk->len)
    {
        me->data_size = offset + blk->len;
        me->data = realloc(me->data, me->data_size);
    }

    memcpy(me->data + offset, blkdata, blk->len);

#if 0 /* debugging */
    {
    int ii;

    printf("diskmem %d:", blk->offset);
    for (ii = 0; ii < blk->len; ii++)
        printf("%02x,", ((const unsigned char*)me->data)[ii]);
    printf("\n");
    }
#endif

    return 1;
}

/**
 * read data.
 * Check if we have the data in the cache;
 * Otherwise retrieve the memory from the disk */
static void *__read_block(
    void *udata,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk
)
{
    diskmem_t *me = udata;
    unsigned int offset;

    offset = blk->piece_idx * me->piece_size + blk->offset;

    if (me->data_size < offset + blk->len)
    {
//        printf("too big %d %d\n", me->data_size, offset + blk->len);
        return NULL;
    }

    return me->data + offset;
}

static int __flush_block(
    void *udata,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk
)
{
    return 0;
}

void *bt_diskmem_new(
)
{
    diskmem_t *me;

    me = calloc(1, sizeof(diskmem_t));
    me->irw.write_block = bt_diskmem_write_block;
    me->irw.read_block = __read_block;
    me->irw.flush_block = __flush_block;
    me->data = NULL;
//    me->irw.giveup_block = NULL;

    return me;
}

void bt_diskmem_free(
    void *meo
)
{
    diskmem_t *me = meo;

    if (me->data)
        free(me->data);
    free(me);
}

void bt_diskmem_set_size(
    void *meo,
    const int piece_bytes_size
)
{
    diskmem_t *me = meo;

    me->piece_size = piece_bytes_size;
    me->data_size = piece_bytes_size * 10;
    me->data = realloc(me->data, me->data_size);
    memset(me->data, 0, me->data_size);
}

bt_blockrw_i *bt_diskmem_get_blockrw(
    void *meo
)
{
    diskmem_t *me = meo;

    return &me->irw;
}

