
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
 *
 * @section description
 * A backend block read/writer that puts data into RAM
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

typedef struct
{
    bt_blockrw_i irw;
    int data_size;
    byte *data;
} diskmem_t;

/*----------------------------------------------------------------------------*/
static int __write_block(
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
)
{
    diskmem_t *dc = udata;

#if 0
    int ii;

    printf("diskmem:");
    for (ii = 0; ii < blk->block_len; ii++)
        printf("%02x,", blkdata[ii]);
    printf("\n");
#endif

    assert(dc->data);
    memcpy(dc->data + blk->block_byte_offset, blkdata, blk->block_len);
    return 1;
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
    diskmem_t *dc = udata;

    return dc->data + blk->block_byte_offset;
}

/*----------------------------------------------------------------------------*/
void *bt_diskmem_new(
)
{
    diskmem_t *dc;

    dc = calloc(1, sizeof(diskmem_t));
    dc->irw.write_block = __write_block;
    dc->irw.read_block = __read_block;
//    dc->irw.giveup_block = NULL;

    return dc;
}

void bt_diskmem_free(
    void *dco
)
{
    diskmem_t *dc = dco;

    if (dc->data)
        free(dc->data);
    free(dc);
}

/*----------------------------------------------------------------------------*/
void bt_diskmem_set_size(
    void *dco,
    const int piece_bytes_size
)
{
    diskmem_t *dc = dco;

    dc->data_size = piece_bytes_size;
    dc->data = realloc(dc->data, dc->data_size);
    memset(dc->data, 0, piece_bytes_size);
}

/*----------------------------------------------------------------------------*/
bt_blockrw_i *bt_diskmem_get_blockrw(
    void *dco
)
{
    diskmem_t *dc = dco;

    return &dc->irw;
}
