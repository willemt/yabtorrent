
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief pwp_msghandler is an adapter between a data channel and pwp_connection
 *        Bytes from data channel are converted into events for pwp_connection
 *        This module allows us to make pwp_connection event based
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include "bitfield.h"
#include "pwp_connection.h"
#include "pwp_msghandler.h"
#include "pwp_msghandler_private.h"

/**
 * Flip endianess **/
static uint32_t fe(uint32_t i)
{
    uint32_t o;
    char *c = (char *)&i;
    char *p = (char *)&o;

    p[0] = c[3];
    p[1] = c[2];
    p[2] = c[1];
    p[3] = c[0];
    return o;
}

int mh_uint32(
        uint32_t* in,
        msg_t *msg,
        const char** buf,
        unsigned int *len)
{
    while (1)
    {
        if (msg->tok_bytes_read == 4)
        {
            *in = fe(*in);
            msg->tok_bytes_read = 0;
            return 1;
        }
        else if (*len == 0)
        {
            return 0;
        }

        *((char*)in + msg->tok_bytes_read) = **buf;
        msg->tok_bytes_read += 1;
        msg->bytes_read += 1;
        *buf += 1;
        *len -= 1;
    }
}

/**
 * @param in Read data into 
 * @param tot_bytes_read Running total of total number of bytes read
 * @param buf Read data from
 * @param len Length of stream left to read from */
int mh_byte(
        char* in,
        unsigned int *tot_bytes_read,
        const char** buf,
        unsigned int *len)
{
    if (*len == 0)
        return 0;

    *in = **buf;
    *tot_bytes_read += 1;
    *buf += 1;
    *len -= 1;
    return 1;
}

int __pwp_piece_data(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int* len)
{
    /* check it isn't bigger than what the message tells
     * us we should be expecting */
    int size = min(*len, m->len - 1 - 4 - 4);

    m->pce.data = *buf;
    m->pce.blk.len = size;
    pwp_conn_piece(me->pc, &m->pce);

    /* If we haven't received the full piece, why don't we
     * just split it "virtually"? That's what we do here: */
    m->len -= size;
    m->pce.blk.offset += size;
    *buf += size;
    *len -= size;

    /* if we received the whole message we're done */
    if (9 == m->len)
    {
        mh_endmsg(me);
    }

    return 1;
}

int __pwp_piece_offset(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int *len)
{
    if (1 == mh_uint32(&m->pce.blk.offset, m, buf, len))
        me->process_item = __pwp_piece_data;
    return 1;
}

int __pwp_piece_pieceidx(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int *len)
{
    if (1 == mh_uint32(&m->pce.blk.piece_idx, m, buf, len))
        me->process_item = __pwp_piece_offset;
    return 1;
}

int __pwp_request_len(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int* len)
{
    if (1 == mh_uint32(&m->blk.len, m, buf, len))
    {
        pwp_conn_request(me->pc, &m->blk);
        mh_endmsg(me);
    }
    return 1;
}

int __pwp_request_offset(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int *len)
{
    if (1 == mh_uint32(&m->blk.offset, m, buf, len))
        me->process_item = __pwp_request_len;
    return 1;
}

int __pwp_request_pieceidx(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int *len)
{
    if (1 == mh_uint32(&m->blk.piece_idx, m, buf, len))
        me->process_item = __pwp_request_offset;
    return 1;
}

int __pwp_cancel_len(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int* len)
{
    if (1 == mh_uint32(&m->blk.len, m, buf, len))
    {
        pwp_conn_cancel(me->pc, &m->blk);
        mh_endmsg(me);
    }
    return 1;
}

int __pwp_cancel_offset(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int *len)
{
    if (1 == mh_uint32(&m->blk.offset, m, buf, len))
        me->process_item = __pwp_cancel_len;
    return 1;
}

int __pwp_cancel_pieceidx(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int *len)
{
    if (1 == mh_uint32(&m->blk.piece_idx, m, buf, len))
        me->process_item = __pwp_cancel_offset;
    return 1;
}

int __pwp_have(pwp_msghandler_private_t *me, msg_t* m, void* udata,
        const char** buf, unsigned int *len)
{
    if (1 == mh_uint32(&m->hve.piece_idx, m, buf, len))
    {
        pwp_conn_have(me->pc, &m->hve);
        mh_endmsg(me);
    }

    return 1;
}

int __pwp_bitfield(pwp_msghandler_private_t *me,
        msg_t* m,
        void* udata,
        const char** buf,
        unsigned int *len)
{
    assert(m->bf.bf->bits);

    /* read and mark bits from 1 byte */
    unsigned char val = 0;
    mh_byte((char*)&val, &m->bytes_read, buf, len);
    unsigned int i;
    for (i=0; i<8; i++)
    {
        if (val & (1 << (7-i)))
        {
            bitfield_mark(m->bf.bf, (m->bytes_read - 5 - 1) * 8 + i);
        }
    }

    /* done reading bitfield */
    if (4 + m->len == m->bytes_read)
    {
        pwp_conn_bitfield(me->pc, &m->bf);
        bitfield_free(m->bf.bf);
        mh_endmsg(me);
    }

    return 1;
}

int __pwp_bitfield_start(pwp_msghandler_private_t *me,
        msg_t* m,
        void* udata,
        const char** buf,
        unsigned int *len)
{
    m->bf.bf = bitfield_new((m->len - 1) * 8);
    me->process_item = __pwp_bitfield;
    return 1;
}

int __pwp_type(pwp_msghandler_private_t *me,
        msg_t* m,
        void* udata __attribute__((unused)),
        const char** buf,
        unsigned int *len)
{
    assert(4 == m->bytes_read);

    mh_byte(&m->id, &m->bytes_read, buf, len);

    /* payloadless messages */
    if (m->len == 1)
    {
        switch (m->id)
        {
        case PWP_MSGTYPE_CHOKE:
            pwp_conn_choke(me->pc);
            break;
        case PWP_MSGTYPE_UNCHOKE:
            pwp_conn_unchoke(me->pc);
            break;
        case PWP_MSGTYPE_INTERESTED:
            pwp_conn_interested(me->pc);
            break;
        case PWP_MSGTYPE_UNINTERESTED:
            pwp_conn_uninterested(me->pc);
            break;
        default: assert(0); break;
        }
        mh_endmsg(me);
    }
    else
    {
        if (me->nhandlers < m->id) 
        {
            printf("ERROR: bad pwp msg type: '%d'\n", m->id);
            mh_endmsg(me);
            return 0;
        }
        /* the next handler can be selected according to the message type */
        assert(0 < m->id);
        assert(m->id < me->nhandlers);
        assert(me->handlers[(int)m->id].func);
        me->process_item = me->handlers[(int)m->id].func;
        me->udata = me->handlers[(int)m->id].udata;
    }

    return 1;
}

int __pwp_length(pwp_msghandler_private_t *me,
        msg_t* m,
        void* udata __attribute__((unused)),
        const char** buf,
        unsigned int *len)
{
    assert(m->bytes_read < 4);

    if (1 == mh_uint32(&m->len, m, buf, len))
    {
        if (0 == m->len)
        {
            pwp_conn_keepalive(me->pc);
            mh_endmsg(me);
        }
        else
        {
            me->process_item = __pwp_type;
        }
    }

    return 1;
}

int pwp_msghandler_dispatch_from_buffer(void *mh,
        const char* buf,
        unsigned int len)
{
    pwp_msghandler_private_t* me = mh;
    msg_t* m = &me->msg;

    /* while we have a stream left to read... */
    while (0 < len)
    {
        assert(me->process_item);
        switch(me->process_item(me,m,me->udata,&buf,&len))
        {
            case 0:
                return 0;
            default:
                break;
        }
    }

    return 1;
}

void mh_endmsg(pwp_msghandler_private_t* me)
{
    me->process_item = __pwp_length;
    me->udata = NULL;
    memset(&me->msg,0,sizeof(msg_t));
}

void* pwp_msghandler_new2(
        void *pc,
        pwp_msghandler_item_t* handlers,
        int nhandlers,
        unsigned int max_workload_bytes)
{
    pwp_msghandler_private_t* me;

    me = calloc(1,sizeof(pwp_msghandler_private_t));
    me->pc = pc;
    me->process_item = __pwp_length;

    int size = PWP_MSGTYPE_CANCEL + 1;
    if (handlers)
        size += nhandlers;
    me->nhandlers = size;
    me->handlers = calloc(1, sizeof(pwp_msghandler_item_t) * size);

    /* add standard bittorrent handlers */
    me->handlers[PWP_MSGTYPE_HAVE].func = __pwp_have;
    me->handlers[PWP_MSGTYPE_BITFIELD].func = __pwp_bitfield_start;
    me->handlers[PWP_MSGTYPE_REQUEST].func = __pwp_request_pieceidx;
    me->handlers[PWP_MSGTYPE_PIECE].func = __pwp_piece_pieceidx;
    me->handlers[PWP_MSGTYPE_CANCEL].func = __pwp_cancel_pieceidx;

    /* add custom user provided handlers */
    int i, s;
    for (i=PWP_MSGTYPE_CANCEL + 1, s=0; handlers && i<size; i++, s++)
    {
        me->handlers[i].func = (void*)handlers[s].func;
        me->handlers[i].udata = handlers[s].udata;
    }
    return me;
}

void* pwp_msghandler_new(void *pc)
{
    return pwp_msghandler_new2(pc,NULL,0,0);
}

void pwp_msghandler_release(void *pc)
{
    free(pc);
}

