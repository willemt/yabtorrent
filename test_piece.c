
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "block.h"
#include "bt.h"
#include "bitfield.h"

/* used for disk memory backend */
#include "bt_diskmem.h"

#include "bt_piece_db.h"
#include "bt_piece.h"
#include "bt_local.h"

#define HASH_EXAMPLE "00000000000000000000"

void TestBTPiece_new_has_set_size_and_hashsum(
    CuTest * tc
)
{
    bt_piece_t *pce;

    pce = bt_piece_new(HASH_EXAMPLE, 40);
    CuAssertTrue(tc, 0 == strncmp(bt_piece_get_hash(pce), HASH_EXAMPLE, 20));
    CuAssertTrue(tc, 40 == bt_piece_get_size(pce));
}

void TestBTPiece_set_idx(
    CuTest * tc
)
{
    bt_piece_t *pce;

    pce = bt_piece_new(HASH_EXAMPLE, 40);
    bt_piece_set_idx(pce, 10);
    CuAssertTrue(tc, 10 == bt_piece_get_idx(pce));
}

void TxestBTPiece_IsDone(
    CuTest * tc
)
{
    bt_piece_t *pce;

    pce = bt_piece_new("00000000000000000000", 40);
//bt_piece_is_complete(
}

void TestBTPiece_full_request_means_piece_is_fully_requested(
    CuTest * tc
)
{
    bt_piece_t *pce;

    bt_block_t req;

    pce = bt_piece_new("00000000000000000000", 40);
    CuAssertTrue(tc, 0 == bt_piece_is_fully_requested(pce));

    bt_piece_poll_block_request(pce, &req);
    CuAssertTrue(tc, req.len == 40);
    CuAssertTrue(tc, 1 == bt_piece_is_fully_requested(pce));
}

void TestBTPiece_unfull_request_means_piece_is_not_fully_requested(
    CuTest * tc
)
{
    bt_piece_t *pce;

    bt_block_t req;

    pce = bt_piece_new("00000000000000000000", 400000);
    bt_piece_poll_block_request(pce, &req);
    CuAssertTrue(tc, req.len == BLOCK_SIZE);
    CuAssertTrue(tc, 0 == bt_piece_is_fully_requested(pce));
}

void TestBTPiece_pollBlockRequest_has_default_blockSize(
    CuTest * tc
)
{
    bt_piece_t *pce;

    bt_block_t req;

    pce = bt_piece_new("00000000000000000000", 400000);
    bt_piece_poll_block_request(pce, &req);
    CuAssertTrue(tc, req.len == BLOCK_SIZE);
}

void TestBTPiece_pollBlockRequest_sized_under_threshhold(
    CuTest * tc
)
{/*----------------------------------------------------------------------------*/
    bt_piece_t *pce;

    bt_block_t req;

    pce = bt_piece_new("00000000000000000000", 10);
    bt_piece_poll_block_request(pce, &req);
    CuAssertTrue(tc, req.len == 10);
}

void TestBTPiece_write_block_needs_disk_blockrw(
    CuTest * tc
)
{
    bt_piece_t *pce;

    bt_block_t blk;

    char *msg = "this great message is 40 bytes in length";

    pce = bt_piece_new("00000000000000000000", 40);
    blk.piece_idx = 0;
    blk.offset = 0;
    blk.len = 10;
    CuAssertTrue(tc, 0 == bt_piece_write_block(pce, NULL, &blk, msg));
}

/**  The below tests will be using the blockrw disk backend to store data.
 *  A mock blockrw has been provided and is defined below: */

#define MAX_MOCKDISK_SIZE 512
typedef struct
{
    char data[MAX_MOCKDISK_SIZE];
} mockdisk_t;

static int __mock_disk_write_block(
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
)
{
    mockdisk_t *md = udata;

    memcpy(md->data, blkdata + blk->offset, blk->len);
}

static void *__mock_disk_read_block(
    void *udata,
    void *caller,
    const bt_block_t * blk
)
{
    mockdisk_t *md = udata;

    return md->data + blk->offset;
}

bt_blockrw_i __mock_disk_rw = {.read_block =
        __mock_disk_read_block,.write_block = __mock_disk_write_block
};

mockdisk_t __mockdisk;

void TestBTPiece_cannot_read_block_we_dont_have(
    CuTest * tc
)
{
    bt_piece_t *pce;

    bt_block_t blk;

    char *msg = "this great message is 40 bytes in length";

    pce = bt_piece_new("00000000000000000000", 40);
    blk.piece_idx = 0;
    blk.offset = 0;
    blk.len = 10;
    bt_piece_set_disk_blockrw(pce, &__mock_disk_rw, &__mockdisk);
    CuAssertTrue(tc, NULL == bt_piece_read_block(pce, NULL, &blk));
}

void TestBTPiece_write_block_means_block_can_be_read(
    CuTest * tc
)
{
    bt_piece_t *pce;

    bt_block_t blk;

    char *msg = "this great message is 40 bytes in length";

    pce = bt_piece_new("00000000000000000000", 40);
    blk.piece_idx = 0;
    blk.offset = 0;
    blk.len = 10;
    bt_piece_set_disk_blockrw(pce, &__mock_disk_rw, &__mockdisk);
    bt_piece_write_block(pce, NULL, &blk, msg);
    CuAssertTrue(tc,
                 0 == strncmp(bt_piece_read_block(pce, NULL, &blk), msg, 10));
}

/*
 *
 * check if the piece is done and valid as per hash (sha1)
 */
void TestBTPiece_doneness_is_valid(
    CuTest * tc
)
{
    bt_piece_t *pce;

    bt_block_t blk;

    char *msg = "this great message is 40 bytes in length";

    char sha1[20] = {
        0xc1, 0x20, 0xbe, 0x68, 0x96, 0x56, 0x82, 0xf8, 0xb1, 0xb0, 0x25,
        0x88, 0x48, 0xa5, 0xfb, 0x04, 0x7f, 0x31, 0x96, 0x1e
    };

    pce = bt_piece_new(sha1, 40);
    bt_piece_set_disk_blockrw(pce, &__mock_disk_rw, &__mockdisk);
    CuAssertTrue(tc, 0 == bt_piece_is_complete(pce));
    blk.piece_idx = 0;
    blk.offset = 0;
    blk.len = 40;
    bt_piece_write_block(pce, NULL, &blk, msg);
    CuAssertTrue(tc, 1 == bt_piece_is_complete(pce));
    CuAssertTrue(tc, 1 == bt_piece_is_valid(pce));
}

void TestBTPiece_Write_Block_To_Stream(
    CuTest * tc
)
{
    bt_piece_t *pce;
    bt_block_t blk;
    void *dc;
    char *msg = "this great message is 40 bytes in length", out[40], *bs;

    bs = out;

    pce = bt_piece_new("00000000000000000000", 40);
    dc = bt_diskmem_new();
    bt_diskmem_set_size(dc, 40);
    bt_piece_set_disk_blockrw(pce, bt_diskmem_get_blockrw(dc), dc);

    /*  write the whole message out */
    blk.piece_idx = 0;
    blk.offset = 0;
    blk.len = 40;
    bt_piece_write_block(pce, NULL, &blk, msg);

    /*  read offset of 10 and length of 20 */
    blk.piece_idx = 0;
    blk.offset = 10;
    blk.len = 20;
    bt_piece_write_block_to_stream(pce, &blk, &bs);
    CuAssertTrue(tc, !strncmp(out, msg + 10, 20));
    bt_piece_free(pce);
    bt_diskmem_free(dc);
}

void TestBTPiece_Write_Block_To_Str(
    CuTest * tc
)
{
#if 0
    bt_piece_t *pce;

    bt_block_t blk;

    char *msg = "this great message is 40 bytes in length", out[40];

    pce = bt_piece_new("00000000000000000000", 40);
    CuAssertTrue(tc, 0 == bt_piece_is_complete(pce));
    blk.offset = 0;
    blk.len = 20;
    bt_piece_write_block(pce, NULL, &blk, msg);
    blk.offset = 10;
    blk.len = 20;
    bt_piece_write_block_to_str(pce, &blk, out);
    CuAssertTrue(tc, 0 == strncmp(out, msg + 10, 10));
#endif
}

#if 0
void TxestBTPiece_BlockIsCompleted(
    CuTest * tc
)
{
    bt_piece_t pce;

    char *msg = "this great message is 40 bytes in length";

    pce = bt_piece_new("00000000000000000000", 40);
    CuAssertTrue(tc, 0 == bt_piece_block_is_completed(&pce, 5, 20));
    bt_piece_add_block(&pce, 5, 20, msg + 5);
    CuAssertTrue(tc, 1 == bt_piece_block_is_completed(&pce, 5, 20));
}
#endif
