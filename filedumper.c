

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bt.h"
#include "bt_local.h"

static void __create_empty_file(
    const char *fname,
    const int size
)
{
#define RANDOM_BITS_LEN 256

    int fd;

    char str[1024];


//        printf("FILELIST: creating %s\n", fname);
    //fd = open(fname, O_CREAT | O_RDWR | O_TRUNC);
    fd = open(fname, O_CREAT | O_RDWR, 0666);
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

void bt_filedumper_write_block(
    bt_filedumper_t * fl,
    const bt_block_t * blk,
    const void *data
)
{
    int bytes_to_write, torr_bytepos;

    bytes_to_write = blk->block_len;
    torr_bytepos = blk->piece_idx * fl->piece_size + blk->block_byte_offset;

//    printf("WRITING %dbytes to %d\n", bytes_to_write, torr_bytepos);

    while (0 < bytes_to_write)
    {
        int fd, file_torr_bytepos, file_pos, write_len;

        file_t *file;

        file = __get_file_from_bytepos(fl, torr_bytepos, &file_torr_bytepos);
        file_pos = torr_bytepos - file_torr_bytepos;
        write_len = bytes_to_write;

        /*  check if it is in the boundary of this file */
        if (file->size < file_pos + write_len)
        {
            write_len = file_pos + write_len - file->size;
        }

        fd = open(file->path, O_RDWR);
        if (-1 == lseek(fd, file_pos, SEEK_SET))
        {
            printf("couldn't seek to %d on file: %s\n", file_pos, file->path);
            perror("couldn't seek\n");
            exit(0);
        }
//        printf("writing %d to %s (%d)\n", write_len, file->path, file_pos);
        write(fd, data, write_len);
        close(fd);

        /*  update our position */
        bytes_to_write -= write_len;
        torr_bytepos += bytes_to_write;
    }
}

void bt_filedumper_read_block(
    bt_filedumper_t * fl,
    bt_block_t * blk,
    void *data_out
)
{
    int bytes_to_read, torr_bytepos;

    void *ptr = data_out;

    bytes_to_read = blk->block_len;
    torr_bytepos = blk->piece_idx * fl->piece_size + blk->block_byte_offset;

    while (0 < bytes_to_read)
    {
        int fd, file_torr_bytepos, file_pos, read_len;

        file_t *file;

        file = __get_file_from_bytepos(fl, torr_bytepos, &file_torr_bytepos);
        file_pos = torr_bytepos - file_torr_bytepos;
        read_len = bytes_to_read;
        /*  check if it is in the boundary of this file */
        if (file->size < file_pos + read_len)
        {
            read_len = file_pos + read_len - file->size;
        }

        fd = open(file->path, O_RDWR);
        if (-1 == lseek(fd, file_pos, SEEK_SET))
        {
            perror("couldn't seek");
            exit(0);
        }
        read(fd, ptr, read_len);
        close(fd);

        /*  update our position */
        bytes_to_read -= read_len;
        ptr += read_len;
        torr_bytepos += bytes_to_read;
    }
}

void bt_filedumper_get_piece_sha1(
    bt_filedumper_t * fl,
    const int piece,
    unsigned int sha1[5]
)
{

}

#if 0
void bt_filedumper_set_func_write(
    bt_filedumper_t * fl,
    int (*__func_write) (void *,
                         void *,
                         void *,
                         int)
)
{

}

void bt_filedumper_set_func_seek(
    bt_filedumper_t * fl,
    int (*__func_read) (void *,
                        void *,
                        int)
)
{

}
#endif
