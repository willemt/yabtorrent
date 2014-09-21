/*
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include "bitfield.h"

unsigned int __bytes_required_for_bits(unsigned int bits)
{
    return (0u < (bits % 8u) ? 1u : 0u) + (bits / 8u);
}

void bitfield_init(bitfield_t * me, const unsigned int nbits)
{
    /* assert(0 < nbits); */
    me->size = nbits;
    me->bits = calloc(1, __bytes_required_for_bits(nbits));
    assert(me->bits);
}

bitfield_t* bitfield_new(const unsigned int nbits)
{
    bitfield_t* me = malloc(sizeof(bitfield_t*));
    bitfield_init(me, nbits);
    return me;
}

void bitfield_clone(bitfield_t * me, bitfield_t * clone)
{
    clone->size = me->size;
    clone->bits = calloc(1, __bytes_required_for_bits(me->size));
    memcpy(clone->bits, me->bits, __bytes_required_for_bits(me->size));
}

void bitfield_free(bitfield_t* me)
{
    free(me);
    free(me->bits);
}

void bitfield_mark(bitfield_t * me, const unsigned int bit)
{
    unsigned int cint;

    assert(me->bits);
    assert(bit < me->size);

    cint = bit / 8u;
    me->bits[cint] |= 1u << (8u - 1u - bit % 8u);
}

void bitfield_unmark(bitfield_t * me, const unsigned int bit)
{
    unsigned int cint;

    assert(me->bits);
    assert(bit < me->size);

    cint = bit / 8u;
    me->bits[cint] &= ~(1u << (8u - 1u - bit % 8u));
}

int bitfield_is_marked(bitfield_t * me, const unsigned int bit)
{
    unsigned int cint;

    assert(me->bits);
    assert(bit < me->size);

    cint = bit / 8u;
    return 0u != (me->bits[cint] & (1u << (8u - 1u - bit % 8u)));
}

unsigned int bitfield_get_length(bitfield_t * me)
{
    return me->size;
}

char *bitfield_str(bitfield_t * me)
{
    char *str = malloc(me->size + 1);

    unsigned int i;
    for (i = 0u; i < me->size; i++)
        str[i] = bitfield_is_marked(me, i) ? '1' : '0';

    str[me->size] = '\0';
    return str;
}
