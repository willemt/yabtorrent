
/**
 * @file
 * @brief block_rw implementor which dumps files to disk
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
            blk->piece_idx * me->piece_size + blk->block_byte_offset,
            blk->block_len,
            data);
}


void *bt_filedumper_read_block(
    void *flo,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk
)
{
    bt_filedumper_t *me = flo;

    return sfa_read(
            me->sfa,
            blk->piece_idx * me->piece_size + blk->block_byte_offset,
            blk->block_len);
}

/*----------------------------------------------------------------------------*/
void *bt_filedumper_new(
)
{
    bt_filedumper_t *me;

    me = calloc(1, sizeof(bt_filedumper_t));
    me->sfa = sfa_new();
    me->irw.write_block = bt_filedumper_write_block;
    me->irw.read_block = bt_filedumper_read_block;
    return me;
}

void bt_filedumper_add_file(
    void* fl,
    const char *fname,
    const int size
)
{
    bt_filedumper_t* me = fl;

    sfa_add_file(me->sfa, fname, size);
}

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

