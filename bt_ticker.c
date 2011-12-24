/*
 * =====================================================================================
 *
 *       Filename:  bt_ticker.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/17/11 23:23:41
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <string.h>
#include <stdarg.h>

#include <arpa/inet.h>

#include "bt.h"
#include "bt_local.h"
#include "url_encoder.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

#include "heap.h"


typedef struct
{
    time_t time;
    heap_t *heap;

} ticker_t;

typedef struct
{
    int trigger_time;
    void *udata;
    void (
    *func
    )    (
    void *
    );
} event_t;

static int __cmp_event(
    const void *i1,
    const void *i2,
    const void *ckr
)
{
    const event_t *p1 = i1;
    const event_t *p2 = i2;

    return p1->trigger_time - p2->trigger_time;
}

void *bt_ticker_new(
)
{
    ticker_t *tk;

    tk = calloc(1, sizeof(ticker_t));
    tk->heap = heap_new(__cmp_event, NULL);
    return tk;
}

/**
 * @param nsecs - run this event in this many seconds
 * @param udata - use this data for the event function
 * @param func - run this event function */
void bt_ticker_push_event(
    void *ti,
    int nsecs,
    void *udata,
    void (*func) (void *)
)
{
    ticker_t *tk = ti;
    event_t *ev;

    ev = malloc(sizeof(event_t));
    ev->trigger_time = tk->time + nsecs;
    ev->udata = udata;
    ev->func = func;
    heap_offer(tk->heap, ev);
}

void bt_ticker_step(
    void *ti
)
{
    ticker_t *tk = ti;

    event_t *ev;

    tk->time = time(NULL);

    ev = heap_peek(tk->heap);

    if (ev && ev->trigger_time < tk->time)
    {
        ev->func(ev->udata);
        free(heap_poll(tk->heap));
    }
}
