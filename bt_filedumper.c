/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief block_rw implementor which dumps files to disk
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 * @section description
 * A backend block read/writer that puts data into RAM
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "block.h"
#include "bt.h"
#include "bt_block_readwriter_i.h"
#include "sparsefile_allocator.h"

/* file dumper */
typedef struct
{
    int piece_size;
    bt_blockrw_i irw;
    void* sfa;
} bt_filedumper_t;

int bt_filedumper_write_block(
    void *flo,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk,
    const void *data
)
{
    bt_filedumper_t *me = flo;

    return sfa_write(
            me->sfa,
            blk->piece_idx * me->piece_size + blk->offset,
            blk->len,
            data);
}

void *bt_filedumper_read_block(
    void *flo,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk
)
{
    bt_filedumper_t *me = flo;

#if 0 /*  debugging */
    printf("fd-readblock: %d %d %d\n",
            blk->piece_idx,  blk->offset, blk->len);
#endif

    return sfa_read(
            me->sfa,
            blk->piece_idx * me->piece_size + blk->offset,
            blk->len);
}

int bt_filedumper_flush_block(
    void *flo,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk
)
{
#if 0
    bt_filedumper_t *me = flo;

    return sfa_write(
            me->sfa,
            blk->piece_idx * me->piece_size + blk->offset,
            blk->len,
            data);
#endif
    return 0;
}

void *bt_filedumper_new(
)
{
    bt_filedumper_t *me;

    me = calloc(1, sizeof(bt_filedumper_t));
    me->sfa = sfa_new();
    assert(me->sfa);
    me->irw.write_block = bt_filedumper_write_block;
    me->irw.read_block = bt_filedumper_read_block;
    me->irw.flush_block = bt_filedumper_flush_block;
    return me;
}

/**
 * Add this file to the bittorrent client
 * This is used for adding new files.
 *
 * @param fname file name
 * @param fname_len length of fname
 * @param flen length in bytes of the file
 * @return 1 on sucess; otherwise 0
 */
void bt_filedumper_add_file(
    void* fl,
    const char *fname,
    int fname_len,
    const int size
)
{
    bt_filedumper_t* me = fl;

    sfa_add_file(me->sfa, fname, fname_len, size);
}

#if 0
int bt_piecedb_add_file(
    bt_piecedb_t * db,
    const char *fname,
    const int fname_len,
    const int flen
)
{
    /* increase total file size by this file's size */
    bt_piecedb_set_tot_file_size(db, bt_piecedb_get_tot_file_size(db) + flen);

    if (priv(db)->func_addfile)
    {
        priv(db)->func_addfile(priv(db)->blockrw_data, fname, flen);
        return 1;
    }

    return 0;
}
#endif

int bt_filedumper_get_nfiles(
    void * fl
)
{
    bt_filedumper_t* me = fl;

    return sfa_get_nfiles(me->sfa);
}

const char *bt_filedumper_file_get_path(
    void * fl,
    const int idx
)
{
    bt_filedumper_t* me = fl;
    return sfa_file_get_path(me->sfa,idx);
}

bt_blockrw_i *bt_filedumper_get_blockrw(
    void * fl
)
{
    bt_filedumper_t* me = fl;

    return &me->irw;
}

void bt_filedumper_set_piece_length(
    void * fl,
    const int piece_size
)
{
    bt_filedumper_t* me = fl;

    me->piece_size = piece_size;
}

void bt_filedumper_set_cwd(
    void * fl,
    const char *path
)
{
    bt_filedumper_t* me = fl;

    sfa_set_cwd(me->sfa, path);
}

