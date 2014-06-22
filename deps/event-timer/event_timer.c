
/**
 * @file
 * @brief Manage timing
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
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

#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

#include "heap.h"

typedef struct
{
    time_t time;
    heap_t *heap;
} eventtimer_t;

typedef struct
{
    int trigger_time;
    void *udata;
    void (*func)( void *);
} event_t;

static int __cmp_event(
    const void *i1,
    const void *i2,
    const void *ckr __attribute__((__unused__))
)
{
    const event_t *p1 = i1;
    const event_t *p2 = i2;

    return p1->trigger_time - p2->trigger_time;
}

void *eventtimer_new(
)
{
    eventtimer_t *tk;

    tk = calloc(1, sizeof(eventtimer_t));
    tk->heap = heap_new(__cmp_event, NULL);
    return tk;
}

void eventtimer_push_event(
    void *ti,
    int nsecs,
    void *udata,
    void (*func) (void *)
)
{
    eventtimer_t *tk = ti;
    event_t *ev;

    ev = malloc(sizeof(event_t));
    ev->trigger_time = tk->time + nsecs;
    ev->udata = udata;
    ev->func = func;
    heap_offer(tk->heap, ev);
}

void eventtimer_step(
    void *ti
)
{
    eventtimer_t *tk = ti;
    event_t *ev;

    tk->time = time(NULL);

    /* poll all due events */
    for (
            ev = heap_peek(tk->heap);
            ev && ev->trigger_time < tk->time;
            ev = heap_peek(tk->heap))
    {
        ev = heap_poll(tk->heap);
        /* run event */
        ev->func(ev->udata);
        /* free event */
        free(ev);
    }
}
