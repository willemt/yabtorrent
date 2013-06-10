
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#include "mock_torrent.h"
#include "mt19937ar.h"
#include "block.h"
#include "bt.h"
#include "bt_local.h"

typedef struct {
    unsigned char *data;
    int size;
    int piece_len;

} mock_torrent_t;

void *mocktorrent_new(int size, int piece_len)
{
    mock_torrent_t* me;
    int ii, *ptr;

    me = malloc(sizeof(mock_torrent_t));
    me->data = malloc(size);
    me->piece_len = piece_len;

    init_genrand(0);

    for (ii=0; ii<size/4; ii++)
    {
        ((int*)me->data)[ii] = genrand_int32();
    }

    return me;
}

void *mocktorrent_get_data(void* _me, unsigned int offset)
{
    mock_torrent_t* me = _me;

    return me->data + offset;
}

void *mocktorrent_get_piece_sha1(void* _me, unsigned int piece)
{
    mock_torrent_t* me = _me;

    return str2sha1hash(me->data + piece * me->piece_len, me->piece_len);
}
