
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


char *ReadFile(
    char *name,
    int *len
)
{
    FILE *file;

    char *buffer;

    unsigned long fileLen;

    //Open file
    file = fopen(name, "rb");
    if (!file)
    {
        fprintf(stderr, "Unable to open file %s", name);
        return;
    }

    //Get file length
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    //Allocate memory
    buffer = (char *) malloc(fileLen + 1);
    if (!buffer)
    {
        fprintf(stderr, "Memory error!");
        fclose(file);
        return;
    }

    //Read file contents into buffer
    fread(buffer, fileLen, 1, file);
    fclose(file);

    *len = fileLen + 1;

    return buffer;

    //Do what ever with buffer

//      free(buffer);
}

void bt_add_tracker_backup(
    int id,
    char *url,
    int url_len
)
{
    printf("backup tracker url: %.*s\n", url_len, url);
}

void bt_set_tracker_url(
    int id,
    char *url,
    int url_len
)
{
    printf("tracker url: %.*s\n", url_len, url);

}

void bt_set_piece_length(
    int id,
    int len
)
{

    printf("piece length: %d\n", len);
}

void bt_add_piece(
    int id,
    const char *piece
)
{
//    printf("got a piece '%.*s'\n", 20, piece);
}

void bt_add_pieces(
    int id,
    const char *pieces,
    int len
)
{
    int prog;

    prog = 0;

    while (prog <= len)
    {
        prog++;

        if (0 == prog % 20)
        {
            //printf("%d\n", prog);
            bt_add_piece(id, pieces);
            pieces += 20;
        }
    }
}

void bt_add_file(
    int id,
    char *fname,
    int fname_len,
    long int flen
)
{
    printf("file: '%.*s' %ld\n", fname_len, fname, flen);
}

void bt_set_path(
    int id,
    const char *path,
    int len
)
{
    printf("setting path: %.*s\n", len, path);
}

void main(
)
{

    int len;

    char *buf;

    buf = ReadFile("test.torrent", &len);
    //buf = ReadFile("meta", &len);

    bt_read_metainfo(0, buf, len);
}
