
/**
Copyright (c) 2011, Willem-Hendrik Thiart
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

#include "stubfile.h"

typedef struct
{
    char *path;
    void *files;
    int nfiles;

    /* total size taken up by files */
    int size;
} sf_t;

typedef struct
{
    const char *path;
    int size;
} file_t;

#if WIN32
static char* strndup(const char* str, const unsigned int len)
{
    char* new;

    new = malloc(len+1);
    strncpy(new,str,len);
    new[len] = '\0';
    return new;
}
#endif

/**
 * Using this global byte pos, get the file that holds this byte
 *
 * @param file_bytepos Position on the stream where the file starts
 * @return the file that the global byte pos is in */
static file_t *__get_file_from_bytepos(
    sf_t * me,
    const int global_bytepos,
    int *file_bytepos
)
{
    int ii, bytes;

    for (ii = 0, bytes = 0; ii < me->nfiles; ii++)
    {
        file_t *file;

        file = &((file_t *) me->files)[ii];

        if (bytes <= global_bytepos && global_bytepos < bytes + file->size)
        {
            *file_bytepos = bytes;
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
    int fd;

//  fd = open(fname, O_CREAT | O_RDWR | O_TRUNC);
    fd = open(fname, O_CREAT | O_RDWR, 0777);

#if 0 /* randomise contents of bits */
#define RANDOM_BITS_LEN 256
    char str[1024];
    int ii;
    for (ii = size; ii > 0; ii -= RANDOM_BITS_LEN)
        write(fd, str, ii < RANDOM_BITS_LEN ? ii : RANDOM_BITS_LEN);
#else /* nice trick to instantly create stub file */
    write(fd, "\0", 1);
    lseek(fd, size - 1, SEEK_SET);
    write(fd, "\0", 1);
#endif

    close(fd);
}

int sf_write(
    void *flo,
    int global_bytepos,
    const int write_len,
    const void *data
)
{
    sf_t *me = flo;
    int bytes_to_write;

    bytes_to_write = write_len;

    while (0 < bytes_to_write)
    {
        int fd, file_bytepos, file_pos, write_len;
        file_t *file;

        /* find file and position we need to write to */
        file = __get_file_from_bytepos(me, global_bytepos, &file_bytepos);
        file_pos = global_bytepos - file_bytepos;
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
            return 0;
        }

        if (-1 == lseek(fd, file_pos, SEEK_SET))
        {
            printf("couldn't seek to %d on file: %s\n", file_pos, file->path);
            perror("couldn't seek\n");
            exit(0);
        }

        if (-1 == (bwrite = write(fd, data, write_len)))
        {
            //perror("Couldn't write to file %s\n", err_str(errno));
            //printf("Couldn't write to file %s\n", strerror(errno));
        }

        if (bwrite == 0)
        {
            printf("couldn't write\n");
            exit(0);
        }

        bytes_to_write -= bwrite;
        global_bytepos += bwrite;
        data += bwrite;

        close(fd);

    }

    return 1;
}

void *sf_read(
    void *flo,
    int global_bytepos,
    int bytes_to_read
)
{
    sf_t *me = flo;
    char *ptr, *data_out;

    ptr = data_out = malloc(bytes_to_read);

#if 0 /* debugging */
    printf("sf-readblock: %d %d\n",
           global_bytepos, bytes_to_read);
#endif

    while (0 < bytes_to_read)
    {
        int file_global_bytepos, file_pos, read_len;
        file_t *file;

        /* find the file to read from */
        if (!(file =
              __get_file_from_bytepos(me, global_bytepos, &file_global_bytepos)))
        { 
            return data_out;
        }

        /* reposition in file */
        file_pos = global_bytepos - file_global_bytepos;
        read_len = bytes_to_read;

#if 0 /* debugging */
        printf("reading file: %s (%dB)\n", file->path, bytes_to_read);
        printf("%d %d %d %d\n", 
                global_bytepos, file_global_bytepos, file_pos, bytes_to_read);
#endif

        int bread = 0;
	FILE *f;

	if (!(f = fopen(file->path, "rb")))
	{
            fprintf(stderr, "Unable to open file %s", file->path);
	}
	
	fseek(f, file_pos, SEEK_SET);

	bread = fread(ptr, 1, read_len, f);
	fclose(f);

        /*  update our position */
        bytes_to_read -= bread;
        ptr += bread;
        global_bytepos += bread;
    }

    return data_out;
}

void *sf_new(
)
{
    sf_t *me;

    me = calloc(1, sizeof(sf_t));
    return me;
}

static int __mkdir(
    char *path,
#if WIN32
    mode_t mode __attribute__((__unused__))
#else
    mode_t mode
#endif
)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
#if WIN32
        if (mkdir(path) != 0)
#else
        if (mkdir(path, mode) != 0)
#endif
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

static int __mkpath(
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
    return 1;
}

void sf_add_file(
    void* sf,
    const char *fname,
    const int fname_len,
    const int size
)
{
    sf_t * me = sf;
    file_t *file;

#if 0 /* debug */
    printf("FILELIST: adding %.*s\n", fname_len, fname);
#endif

    me->nfiles += 1;
    me->size += size;
    assert(0 < me->nfiles);

    /* increase size of array */
    me->files = realloc(me->files, me->nfiles * sizeof(file_t));

    /* add file */
    file = &((file_t *) me->files)[me->nfiles - 1];
    file->path = strndup(fname,fname_len);
    file->size = size;

    /* create physical file */
    __mkpath(file->path, 0777);
    if (!__exists(file->path) || __filesize(file->path) != size)
    {
        __create_empty_file(file->path, size);
    }
}

int sf_get_nfiles(
    void* sf
)
{
    sf_t* me = sf;

    return me->nfiles;
}

const char *sf_file_get_path(
    void* sf,
    int idx
)
{
    sf_t* me = sf;
    file_t *file;

    assert(0 <= idx && idx < me->nfiles);
    file = &((file_t *) me->files)[idx];
    return file->path;
}

void sf_set_cwd(
    void* sf,
    const char *path
)
{
    sf_t* me = sf;

    me->path = strdup(path);
    if (0 != chdir(me->path))
    {
        printf("directory: %s\n", me->path);
        perror("could not change directory");
        exit(0);
    }
}

unsigned int sf_get_total_size(void * sf)
{
    sf_t* me = sf;
    return me->size;
}

