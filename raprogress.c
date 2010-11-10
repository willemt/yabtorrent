#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "raprogress.h"

typedef struct var_block_s var_block_t;

struct var_block_s
{
    int offset;
    int len;
    var_block_t *next;
};

static int __capmax(
    int val,
    int max
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

void *raprogress_init(
    const int max
)
{
    raprogress_t *prog;

    prog = calloc(1, sizeof(raprogress_t));

    prog->max = max;
    prog->first_block = NULL;

    return prog;
}

int raprogress_get_num_blocks(
    raprogress_t * prog
)
{
    var_block_t *block;

    int num;

//    printf("\ncounting blocks\n");

    block = prog->first_block;

    for (num = 0; block; num++)
    {
//        printf("block: %d %d\n", block->offset, block->len);
        block = block->next;
    }

//    printf("ending %d\n", num);

    return num;
}

void raprogress_mark_complete(
    raprogress_t * prog,
    int offset,
    int len
)
{
    if (!prog->first_block)
    {
        var_block_t *block;

        block = prog->first_block = malloc(sizeof(var_block_t));
        block->len = len;
        block->offset = offset;
        block->next = NULL;
        return;
    }

    var_block_t *this, *prev;

    /* get 1st block that has a higher offset than us */

    prev = prog->first_block;

    if (offset < prev->offset)
    {
        var_block_t *block;

        block = prog->first_block = malloc(sizeof(var_block_t));
        block->len = len;
        block->offset = offset;
        block->next = prev;
        this = prev;
        prev = block;
    }
    else
    {
        while (prev->next && (offset >= prev->next->offset))
        {
            prev = prev->next;
        }

        this = prev->next;

        /* The left block eats/touches this new one...
         * |00000LLN00000|;
         * |00000LLNL0000|
         * Combine the old with new */
        if (offset <= prev->offset + prev->len)
        {
            /* completely ate this one!
             * |00000LLNL0000| */
            if (offset + len <= prev->offset + prev->len)
            {
                return;
            }

            /* increase coverage */
            prev->len = (offset + len) - prev->offset;
            /* might eat another... */
        }
        else
        {
            var_block_t *blk;

            blk = malloc(sizeof(var_block_t));
            blk->offset = offset;
            blk->len = len;
            blk->next = this;
            prev->next = blk;
            prev = blk;
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

bool raprogress_is_complete(
    raprogress_t * prog
)
{
    var_block_t *block;

    block = prog->first_block;

    return block && !block->next && block->len == prog->max;
}

void raprogress_get_incomplete(
    raprogress_t * prog,
    int *offset,
    int *len,
    const int max
)
{
    var_block_t *block;

    *len = 0;
    *offset = 0;

    block = prog->first_block;

    if (!block)
    {
        *offset = 0;
        *len = max;
    }
    else
    {
        if (block->offset != 0)
        {
            *offset = 0;

            if (block->next)
            {
                *len = block->next->offset - block->offset;
            }
            else
            {
                *len = block->offset;
            }
        }
        else if (!block->next)
        {
            *offset = block->len;
            *len = max;
        }
        else
        {
            int start;

            *offset = start = 0 + block->len;
            *len = block->next->offset - block->len;
        }
    }

    *len = __capmax(*len, max);
}
