/*
 * =====================================================================================
 *
 *       Filename:  test_filedumper.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/06/11 15:41:52
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "bt.h"
#include "bt_local.h"

typedef struct
{
    int pos;
} __fd_t;

typedef struct
{
    char data[1024];
    __fd_t cursors[10];
} __writer_t;

static int __func_write(
    __writer_t * writer,
    void *handler,
    void *data,
    int len
)
{


}

static int __func_seek(
    __writer_t * writer,
    void *handler,
    int offset
)
{
    __fd_t *fd = handler;

    fd->pos += offset;
}

static int __func_open(
    __writer_t * writer,
    void *handler,
    int flags
)
{


}

void Testfiledumper_WriteBlock(
    CuTest * tc
)
{
#if 0
    __writer_t writer;

    bt_filedumper_t *fl;

    bt_block_t blk;

    int file;

    char *data = "data";


    fl = bt_filedumper_new();
    __clear(&writer);
//    bt_filedumper_set_func_read(fl,__func_read);
    bt_filedumper_set_path(fl, "./");
    bt_filedumper_set_udata(fl, &writer);
    bt_filedumper_set_func_write(fl, __func_write);
    bt_filedumper_set_func_seek(fl, __func_seek);
    bt_filedumper_set_piece_size(fl, 50);
    bt_filedumper_add_file(fl, "test.xml", 100);
    blk.piece_idx = 0;
    blk.block_byte_offset = 0;
    blk.block_len = 4;
    bt_filedumper_write_block(fl, &blk, data);
    CuAssertTrue(tc, 0 == strncpy(writer.data, "data", 4));
#endif
}

void Testfiledumper_ReadBlock(
    CuTest * tc
)
{
#if 0
    __writer_t writer;

    bt_filedumper_t *fl;

    bt_block_t blk;

    int file;

    char out[256];

    fl = bt_filedumper_new();
    __clear(&writer);
    sprintf(writer.data, "this is testing data\n");
    bt_filedumper_set_path(fl, "./");
    bt_filedumper_set_func_read(fl, __func_read);
    bt_filedumper_set_func_seek(fl, __func_seek);
    bt_filedumper_set_udata(fl, &writer);
    bt_filedumper_set_piece_size(fl, 50);
    blk.piece_idx = 0;
    blk.block_byte_offset = 4;
    blk.block_len = 4;
    bt_filedumper_read_block(fl, &blk, out);
    CuAssertTrue(tc, 0 == strncpy(data, " is ", 4));
#endif
}

void Testfiledumper_PieceSha1(
    CuTest * tc
)
{
#if 0
    __writer_t writer;

    bt_filedumper_t *fl;

    int file;

    char *data = "data";

    unsigned int sha1[5];

    unsigned int sha1b[5];

    fl = bt_filedumper_new();
    __clear(&writer);
    bt_filedumper_set_udata(fl, &writer);
    bt_filedumper_set_func_read(fl, __func_read);
    bt_filedumper_set_func_seek(fl, __func_seek);
    bt_filedumper_set_piece_size(fl, 50);
    bt_filedumper_add_file(fl, "test.xml", 100);
    bt_filedumper_get_piece_sha1(fl, 1, sha1);
    CuAssertTrue(tc, 1 == bt_filedumper_piece_is_valid(fl, 0));

    SHA1Context sha;

    SHA1Reset(&sha);
    SHA1Input(&sha, pce->data, pce->data_size);
    SHA1Result(&sha);
#endif
}

/*----------------------------------------------------------------------------*/


void Testfiledumper_new_is_Empty(
    CuTest * tc
)
{
    bt_filedumper_t *fl;

    fl = bt_filedumper_new();
    CuAssertTrue(tc, NULL != fl);
}

void Testfiledumper_Add_File_gets_added(
    CuTest * tc
)
{
    bt_filedumper_t *fl;

    fl = bt_filedumper_new();
    CuAssertTrue(tc, 0 == bt_filedumper_get_nfiles(fl));

    bt_filedumper_add_file(fl, "test.xml", 100);
    CuAssertTrue(tc, 1 == bt_filedumper_get_nfiles(fl));
}

void Testfiledumper_Get_file_From_Block(
    CuTest * tc
)
{
    bt_filedumper_t *fl;

    bt_block_t blk;

    int file;

    blk.piece_idx = 0;
    blk.block_byte_offset = 0;
    blk.block_len = 50;

    fl = bt_filedumper_new();
    bt_filedumper_set_piece_length(fl, 50);

    bt_filedumper_add_file(fl, "test.xml", 100);
    CuAssertTrue(tc, 1 == bt_filedumper_get_nfiles(fl));

//    file = bt_filedumper_get_file_from_block(fl,&blk);
}

void Testfiledumper_Added_Filepath_can_be_gotten_by_idx(
    CuTest * tc
)
{
    bt_filedumper_t *fl;

    fl = bt_filedumper_new();
    bt_filedumper_add_file(fl, "test.xml", 100);
    bt_filedumper_add_file(fl, "x.xml", 100);
    CuAssertTrue(tc, !strcmp("test.xml", bt_filedumper_file_get_path(fl, 0)));
}

void Txestfiledumper_WriteBlock_cant_happen_if_file_too_small(
    CuTest * tc
)
{
#if 0
    __writer_t writer;

    bt_filedumper_t *fl;

    bt_block_t blk;

    int file;

    char *data = "data";

    fl = bt_filedumper_new();
    __clear(&writer);
    bt_filedumper_set_path(fl, "./");
    bt_filedumper_set_udata(fl, &writer);
    bt_filedumper_set_func_write(fl, __func_write);
    bt_filedumper_set_func_seek(fl, __func_seek);
    bt_filedumper_set_piece_size(fl, 50);
    bt_filedumper_add_file(fl, "test.xml", 10);
    blk.piece_idx = 0;
    blk.block_byte_offset = 0;
    blk.block_len = 11;
    CuAssertTrue(tc, 0 == bt_filedumper_write_block(fl, NULL, &blk, data));
//    CuAssertTrue(tc, 0 == strncpy(writer.data, "data", 4));
#endif
}
