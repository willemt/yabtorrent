
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "mock_torrent.h"
#include "mt19937ar.h"
#include "bt.h"
#include "bt_sha1.h"

typedef struct {
    char *data;
    int size;
    int piece_len;

} mock_torrent_t;

void *mocktorrent_new(int size, int piece_len)
{
    mock_torrent_t* me;
    int ii;

    me = malloc(sizeof(mock_torrent_t));
    me->data = malloc(size * piece_len);
    me->piece_len = piece_len;

    init_genrand(0);

    /* create random data */
    for (ii=0; ii < (size * piece_len)/sizeof(int); ii++)
    {
        ((int*)me->data)[ii] = genrand_int32();
    }

    return me;
}

void *mocktorrent_get_data(void* _me, unsigned int piece)
{
    mock_torrent_t* me = _me;

    return me->data + piece * me->piece_len;
}

void *mocktorrent_get_piece_sha1(void* _me, char* hash, unsigned int piece)
{
    mock_torrent_t* me = _me;

    bt_str2sha1hash(hash, me->data + piece * me->piece_len, me->piece_len);
    return hash;
}
