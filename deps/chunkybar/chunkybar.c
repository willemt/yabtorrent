#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "chunkybar.h"

typedef struct var_chunk_s var_chunk_t;

struct var_chunk_s
{
    unsigned int offset;
    unsigned int len;
    var_chunk_t *next;
};

static unsigned int __capmax(
    unsigned int val,
    unsigned int max
)
{
    if (max < val)
    {
        return max;
    }
    else
    {
        return val;
    }
}

void *chunky_new(const unsigned int max)
{
    chunkybar_t *me;

    me = calloc(1, sizeof(chunkybar_t));
    me->max = max;
    me->first_chunk = NULL;

    assert(me);
    return me;
}

void chunky_free(
    void *ra
)
{
    chunkybar_t *me = ra;
    var_chunk_t *b, *prev;

    for (b = me->first_chunk; b; )
    {
        prev = b;
        b=b->next;
        free(prev);
    }

    free(me);
}

void chunky_set_max(chunkybar_t* me, const unsigned int max)
{
    me->max = max;
}

void chunky_mark_all_incomplete(chunkybar_t * me)
{
    chunky_mark_incomplete(me,0,me->max);
}

int chunky_get_num_chunks(const chunkybar_t * me)
{
    const var_chunk_t *b;
    int num;

    for (num=0, b = me->first_chunk; b; num++, b = b->next);

    return num;
}

void chunky_mark_complete(
    chunkybar_t * me,
    const unsigned int offset,
    const unsigned int len
)
{
    var_chunk_t *this = NULL, *prev;

    /* initialise first chunk */
    if (!me->first_chunk)
    {
        var_chunk_t *b;

        b = malloc(sizeof(var_chunk_t));
        b->len = len;
        b->offset = offset;
        b->next = NULL;

        /* set the node in one moment */
        me->first_chunk = b;
        return;
    }

    /* get 1st chunk that has a higher offset than us */

    prev = me->first_chunk;

    /* we are at the beginning of the chunks */
    if (offset < prev->offset)
    {
        var_chunk_t *b;

        b = malloc(sizeof(var_chunk_t));
        b->len = len;
        b->offset = offset;
        b->next = this = prev;
        prev = b;

        /* set the node in one moment */
        me->first_chunk = b;
    }
    else
    {
        /*  find a place where our offset is equal or greater */
        while (prev->next && (prev->next->offset <= offset))
        {
            prev = prev->next;
        }

        this = prev->next;

        /* The left chunk eats/touches this new one...
         * |00000LLN00000|;
         * |00000LLNL0000|
         * Combine the old with new */
        if (offset <= prev->offset + prev->len)
        {
            /* completely ate this one!
             * |00000LLNL0000| */
            if (offset + len <= prev->offset + prev->len)
            {

            }
            else
            {
                /* increase coverage */
                prev->len = (offset + len) - prev->offset;
                /* might eat another... */
            }
        }
        else
        {
            var_chunk_t *b;

            b = malloc(sizeof(var_chunk_t));
            b->offset = offset;
            b->len = len;
            b->next = this;
            prev->next = b;
            prev = b;
        }
    }

    /* prev will feast! */
    if (this && this->offset <= prev->offset + prev->len)
    {
        do
        {
            /* not eaten */
            if (!(this->offset + this->len <= prev->offset + prev->len))
            {
                prev->len = (this->offset + this->len) - prev->offset;
            }

            prev->next = this->next;
            free(this);
            this = prev->next;
        }
        while (this && this->offset <= prev->offset + prev->len);
    }
}

void chunky_mark_incomplete(
    chunkybar_t * me,
    const unsigned int offset,
    const unsigned int len
)
{
    var_chunk_t *this = NULL, *prev;

    if (!me->first_chunk)
        return;

    /* get 1st chunk that has a higher offset than us */

    prev = NULL;
    this = me->first_chunk;

    while (this)
    {
        /*  whole chunk gets eaten */
        if (offset <= this->offset &&
                this->offset + this->len <= offset + len)
        {
            if (prev)
            {
                prev->next = this->next;
                free(this);
                this = prev;
            }
            else
            {
                me->first_chunk = this->next;
                free(this);
                this = me->first_chunk;
                if (!this) break;
            }
        }
        /*
         * In the middle
         * |00000LXL00000|
         */
        else if (this->offset < offset &&
                offset + len < this->offset + this->len)
        {
            var_chunk_t *b;

            b = malloc(sizeof(var_chunk_t));
            b->offset = offset + len;
            b->len = this->len - len - (offset - this->offset);
            b->next = this->next;

            this->len = offset - this->offset;
            this->next = b;
        }
        /*
         * swallow left
         * |00000XLL00000|
         */
        else if (this->offset < offset + len &&
                offset + len < this->offset + this->len)
        {
            this->len -= offset+len - this->offset;
            this->offset = offset+len;
        }
        /*
         * swallow right
         * |00000LLX00000|
         */
        else if (this->offset < offset &&
                offset < this->offset + this->len &&
                this->offset + this->len <= offset + len)
        {
            this->len = offset - this->offset;
        }

        prev = this;
        this = this->next;
    }
}

int chunky_is_complete(const chunkybar_t * me)
{
    const var_chunk_t *b;

    b = me->first_chunk;
    return b && !b->next && b->len == me->max;
}

void chunky_get_incomplete(
    const chunkybar_t * me,
    unsigned int *offset,
    unsigned int *len,
    const unsigned int max
)
{
    const var_chunk_t *b;

    *offset = *len = 0;
    b = me->first_chunk;

    if (!b)
    {
        *offset = 0;
        *len = max;
    }
    else
    {
        if (b->offset != 0)
        {
            *offset = 0;

            if (b->next)
            {
                *len = b->next->offset - b->offset;
            }
            else
            {
                *len = b->offset;
            }
        }
        else if (!b->next)
        {
            *offset = b->len;
            *len = max;
        }
        else
        {
            *offset = 0 + b->len;
            *len = b->next->offset - b->len;
        }
    }

    *len = __capmax(*len, max);

//    printf("%d %d %d %d\n", me->max, *len, max, *offset + *len);

    /*  make sure we aren't going over the boundary */
    if (me->max < *offset + *len)
    {
        *len = me->max - *offset;
    }
}

unsigned int chunky_get_nbytes_completed(
    const chunkybar_t * me
)
{
    const var_chunk_t *b;
    unsigned int nbytes;

    for (b = me->first_chunk, nbytes=0; b; b = b->next)
    {
        nbytes += b->len;
    }

    return nbytes;
}

int chunky_have(
    const chunkybar_t * me,
    const unsigned int offset,
    const unsigned int len
)
{
    const var_chunk_t *b;

    for (b = me->first_chunk; b; b = b->next)
    {
        if (offset < b->offset)
        {

        }
        else if (b->offset <= offset)
        {
            if (offset + len <= b->offset + b->len)
            {
                return 1;
            }
        }
        else
        {
            return 0;
        }
    }

    return 0;
}

void chunky_print_contents(const chunkybar_t * me)
{
    const var_chunk_t *b;

    for (b = me->first_chunk; b; b = b->next)
    {
        printf("%d to %d\n", b->offset, b->offset + b->len);
    }
}
