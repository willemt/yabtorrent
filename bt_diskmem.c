/*
 * =====================================================================================
 *
 *       Filename:  bt_diskmem.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/14/11 02:09:35
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

/**
 * 
 * A backend block read/writer that puts data into RAM
 *
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
