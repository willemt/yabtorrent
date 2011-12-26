
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "bt.h"
#include "bt_local.h"

typedef struct
{
    const char *path;
    int size;

} file_t;

/**
 * using this torrent byte pos, get the file that holds this byte */
static file_t *__get_file_from_bytepos(
    bt_filedumper_t * fl,
    const int torr_bytepos,
    /*  position on the stream where the file starts */
    int *file_torr_bytepos
)
{
    int ii, bytes;

    for (ii = 0, bytes = 0; ii < fl->nfiles; ii++)
    {
        file_t *file;

        file = &((file_t *) fl->files)[ii];

        if (bytes <= torr_bytepos && torr_bytepos < bytes + file->size)
        {
            *file_torr_bytepos = bytes;
            return file;
        }

        bytes += file->size;
    }

    return NULL;
}

static void __create_empty_file(
    const char *fname,
    const int size
)
{
#define RANDOM_BITS_LEN 256

    int fd;

    char str[1024];

//  printf("FILELIST: creating %s\n", fname);
//  fd = open(fname, O_CREAT | O_RDWR | O_TRUNC);
    fd = open(fname, O_CREAT | O_RDWR, 0777);
#if 0
    int ii;

    int rand_num;

    for (ii = size; ii > 0; ii -= RANDOM_BITS_LEN)
    {
//        rand_num = random();
//        sprintf(str, "%32d", rand_num);
        write(fd, str, ii < RANDOM_BITS_LEN ? ii : RANDOM_BITS_LEN);
    }
#else
    write(fd, "\0", 1);
    lseek(fd, size - 1, SEEK_SET);
    write(fd, "\0", 1);
#endif

    close(fd);
}

int bt_filedumper_write_block(
    void *flo,
    void *caller,
    const bt_block_t * blk,
    const void *data
)
{
    bt_filedumper_t *fl = flo;

    int bytes_to_write, torr_bytepos;

    bytes_to_write = blk->block_len;
    torr_bytepos = blk->piece_idx * fl->piece_size + blk->block_byte_offset;

//    printf(">>>\n");
//    printf("WRITING %dB to %d\n", bytes_to_write, torr_bytepos);

    while (0 < bytes_to_write)
    {
        int fd, file_torr_bytepos, file_pos, write_len;

        file_t *file;

        file = __get_file_from_bytepos(fl, torr_bytepos, &file_torr_bytepos);
        file_pos = torr_bytepos - file_torr_bytepos;
        write_len = bytes_to_write;

        if (!file)
            return 1;

        assert(file);

        /*  check if it is in the boundary of this file */
        if (file->size < file_pos + write_len)
        {
            write_len = file->size - file_pos;
        }

        int bwrite = 0;

        if (-1 == (fd = open(file->path, O_RDWR, 0777)))
        {
            perror("couldn't open file");
//            exit(0);
//            goto skip;
            return 0;
        }

        if (-1 == lseek(fd, file_pos, SEEK_SET))
        {
            printf("couldn't seek to %d on file: %s\n", file_pos, file->path);
            perror("couldn't seek\n");
            exit(0);
        }

//        printf("writing %d: %d to %s (%d,%d,fs:%d/%d)\n", torr_bytepos,
//               write_len, file->path, file_pos, fd, file_pos, file->size);

        if (-1 == (bwrite = write(fd, data, write_len)))
        {
            perror("couldn't write\n");
            exit(0);
        }


        if (bwrite < write_len)
        {
//            printf("not enough space: %d/%d\n", bwrite, write_len);
//            exit(0);
//            return 0;
        }

        if (0 == bwrite)
        {
            perror("couldn't write");
            exit(0);
        }

        close(fd);

        /*  update our position */
        bytes_to_write -= bwrite;
        torr_bytepos += bwrite;
//        printf("wrote:%d left:%d pos:%d\n", bwrite, bytes_to_write,
//               torr_bytepos);
    }

    return 1;
}


void *bt_filedumper_read_block(
    void *flo,
    void *caller,
    const bt_block_t * blk
)
{
    bt_filedumper_t *fl = flo;
    int bytes_to_read, torr_bytepos;

    char *ptr, *data_out;

    bytes_to_read = blk->block_len;
    torr_bytepos = blk->piece_idx * fl->piece_size + blk->block_byte_offset;
    ptr = data_out = malloc(bytes_to_read);
#if 0
    printf("fd-readblock: %d %d %d\n", blk->piece_idx,
           blk->block_byte_offset, blk->block_len);
#endif
    while (0 < bytes_to_read)
    {
        int fd, file_torr_bytepos, file_pos, read_len;

        file_t *file;

        if (!(file =
              __get_file_from_bytepos(fl, torr_bytepos, &file_torr_bytepos)))
        {
            return data_out;
        }

//        printf(" reading file: %s (%dB)\n", file->path, bytes_to_read);
        file_pos = torr_bytepos - file_torr_bytepos;
        read_len = bytes_to_read;
#if 0
        /*  check if it is in the boundary of this file */
        if (file->size < file_pos + read_len)
        {
            printf("read shorted\n");
            read_len = file_pos + read_len - file->size;
        }
#endif
        int bread = 0;

        if (-1 == (fd = open(file->path, O_RDWR)))
        {
//            perror("couldn't open file");
//            exit(0);
            goto skip;
        }

        if (-1 == lseek(fd, file_pos, SEEK_SET))
        {
            perror("couldn't seek");
            exit(0);
        }


        if (-1 == (bread = read(fd, ptr, read_len)))
        {
            perror("couldn't read");
            exit(0);
        }

        if (0 == bread)
        {

        }

//        printf("  bread: %d  readlen: %d\n", bread, read_len);

        close(fd);
//        printf("data_out[0] = %x\n", data_out[0]);
      skip:
        /*  update our position */
        bytes_to_read -= bread;
        ptr += bread;
        torr_bytepos += bread;
    }

    return data_out;
}

/*----------------------------------------------------------------------------*/
bt_filedumper_t *bt_filedumper_new(
)
{
    bt_filedumper_t *fl;

    fl = calloc(1, sizeof(bt_filedumper_t));
    fl->irw.write_block = bt_filedumper_write_block;
    fl->irw.read_block = bt_filedumper_read_block;
    return fl;
}

static int __mkdir(
    char *path,
    mode_t mode
)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
        if (mkdir(path, mode) != 0)
            return -1;
    }

    return 1;
}

static int __filesize(
    const char *fname
)
{
    struct stat st;

    stat(fname, &st);
    return st.st_size;
}

static int __exists(
    const char *fname
)
{
    struct stat buffer;

    return (stat(fname, &buffer) == 0);
}

int mkpath(
    const char *path,
    mode_t mode
)
{
    char *marker = strdup(path);

    char *sp, *pp;

    sp = pp = marker;
    while (NULL != (sp = strchr(pp, '/')))
    {
        *sp = '\0';
        __mkdir(marker, mode);
        *sp = '/';
        pp = sp + 1;
    }

    free(marker);
//    __mkdir(pp, mode);
    return 1;
}

void bt_filedumper_add_file(
    bt_filedumper_t * fl,
    const char *fname,
    const int size
)
{
    file_t *file;

//    printf("FILELIST: adding %s\n", fname);
    fl->nfiles += 1;
    assert(0 < fl->nfiles);
    fl->files = realloc(fl->files, fl->nfiles * sizeof(file_t));
    file = &((file_t *) fl->files)[fl->nfiles - 1];
    file->path = fname;
    file->size = size;
#if 1
    mkpath(file->path, 0777);
    if (!__exists(file->path) || __filesize(file->path) != size)
    {
        __create_empty_file(file->path, size);
    }
#endif

//    if (0 == chdir(fl->path))

}

/*----------------------------------------------------------------------------*/

int bt_filedumper_get_nfiles(
    bt_filedumper_t * fl
)
{
    return fl->nfiles;
}

const char *bt_filedumper_file_get_path(
    bt_filedumper_t * fl,
    int idx
)
{
    file_t *file;

    assert(0 <= idx && idx < fl->nfiles);
    file = &((file_t *) fl->files)[idx];
    return file->path;
}

bt_blockrw_i *bt_filedumper_get_blockrw(
    bt_filedumper_t * fl
)
{
    return &fl->irw;
}

/*----------------------------------------------------------------------------*/
void bt_filedumper_set_piece_length(
    bt_filedumper_t * fl,
    const int piece_size
)
{
    fl->piece_size = piece_size;
}

void bt_filedumper_set_path(
    bt_filedumper_t * fl,
    const char *path
)
{
    fl->path = strdup(path);
    if (0 != chdir(fl->path))
    {
        printf("directory: %s\n", fl->path);
        perror("could not change directory");
        exit(0);
    }
}

void bt_filedumper_set_udata(
    bt_filedumper_t * fl,
    void *udata
)
{

}
