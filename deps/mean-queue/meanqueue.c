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

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include "meanqueue.h"

void *meanqueue_new(
    const int size
)
{
    meanqueue_t *qu;

    qu = calloc(1, sizeof(meanqueue_t));
    qu->vals = calloc(size, sizeof(int));
    qu->size = size;
    return qu;
}

void meanqueue_free(
    meanqueue_t * qu
)
{
    free(qu->vals);
    free(qu);
}

void meanqueue_offer(
    meanqueue_t * qu,
    const int val
)
{
    qu->valSum += val;
    qu->valSum -= qu->vals[qu->idxCur];
    qu->vals[qu->idxCur] = val;
    qu->idxCur++;
    qu->mean = (float) qu->valSum / qu->size;
    if (qu->idxCur >= qu->size)
        qu->idxCur = 0;
}

float meanqueue_get_value(
    meanqueue_t * qu
)
{
    return qu->mean;
}
