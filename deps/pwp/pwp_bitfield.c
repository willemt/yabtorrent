
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* for uint32_t */
#include <stdint.h>

#include "bitfield.h"
#include "pwp_connection.h"
#include "pwp_local.h"
#include "bitstream.h"
#include "chunkybar.h"

int pwp_send_bitfield(
        int npieces,
        void* pieces_completed,
        func_send_f send_cb,
        void* cb_ctx,
        void* peer_udata
        )
{
    // TODO: replace 1000
    char data[1000], *ptr;
    uint32_t size;

    ptr = data;
    size = sizeof(uint32_t) + sizeof(char) + (npieces / 8) +
        ((npieces % 8 == 0) ? 0 : 1);
    bitstream_write_uint32(&ptr, fe(size - sizeof(uint32_t)));
    bitstream_write_byte(&ptr, PWP_MSGTYPE_BITFIELD);

    /*  for all pieces set bit = 1 if we have the completed piece */
    int i;
    unsigned char bits;
    for (bits = 0, i = 0; i < npieces; i++)
    {
        bits |= chunky_have(pieces_completed, i, 1) << (7 - (i % 8));
        /* ...up to eight bits, write to byte */
        if (((i + 1) % 8 == 0) || npieces - 1 == i)
        {
            bitstream_write_byte(&ptr, (char)bits);
            bits = 0;
        }
    }

    return send_cb(cb_ctx, peer_udata, data, size);
}

