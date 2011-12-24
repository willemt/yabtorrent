/*
 * =====================================================================================
 *
 *       Filename:  bt_bitfield.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/06/11 11:37:36
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

#include <stdint.h>

#include "bt.h"
#include "bt_local.h"

void bt_bitfield_init(
    bt_bitfield_t * bf,
    const int nbits
)
{
//    assert(0 < nbits);
    bf->bits = calloc(nbits, sizeof(uint32_t));
    bf->size = nbits;
//    bf->bits = realloc(bf->bits, sizeof(uint32_t) * nbits);
}

void bt_bitfield_mark(
    bt_bitfield_t * bf,
    const int bit
)
{
    int cint;

    assert(bf->bits);
    assert(0 <= bit);
    assert(bit < bf->size);

    cint = bit / 32;
    bf->bits[cint] |= 1 << (31 - bit % 32);
}

void bt_bitfield_unmark(
    bt_bitfield_t * bf,
    const int bit
)
{
    assert(bf->bits);
    assert(0 <= bit);
    assert(bit < bf->size);


}

int bt_bitfield_is_marked(
    bt_bitfield_t * bf,
    const int bit
)
{
    assert(bf->bits);
    assert(0 <= bit);
    assert(bit < bf->size);

    int cint;

    cint = bit / 32;
    return 0 != (bf->bits[cint] & (1 << (31 - bit % 32)));
}

int bt_bitfield_get_length(
    bt_bitfield_t * bf
)
{
    return bf->size;
}

char *bt_bitfield_str(
    bt_bitfield_t * bf
)
{
    char *str;

    int ii;

    str = malloc(bf->size + 1);

    for (ii = 0; ii < bf->size; ii++)
    {
        str[ii] = bt_bitfield_is_marked(bf, ii) ? '1' : '0';
    }

    str[bf->size] = '\0';
    return str;
}
