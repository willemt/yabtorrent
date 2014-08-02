
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Manage a connection with a peer
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "bitfield.h"
#include "pwp_connection.h"
#include "pwp_handshaker.h"
#include "pwp_local.h"
#include "bitstream.h"

typedef struct {
    pwp_handshake_t hs;
    unsigned int bytes_read;

    char* cur;
    char* curr_value;

    /* expected infohash */
    char* expected_ih;

    /* my peer id */
    char* my_pi;

    /* 1 is handshake is done; 0 otherwise */
//    int status;
} pwp_handshaker_t;

int pwp_send_handshake(
        void* callee,
        void* udata,
        int (*send)(void *callee, const void *udata, const void *send_data, const int len),
        char* expected_ih,
        char* my_pi)
{
    char buf[1024], *protocol_name = PROTOCOL_NAME, *ptr;
    int size, ii;

    assert(NULL != expected_ih);
    assert(NULL != my_pi);

//    sprintf(buf, "%c%s" PWP_PC_HANDSHAKE_RESERVERD "%s%s",
//            strlen(protocol_name), protocol_name, expected_ih, peerid);

    ptr = buf;

    /* protocol name length */
    bitstream_write_byte((char**)&ptr, strlen(protocol_name));

    /* protocol name */
    bitstream_write_string((char**)&ptr, protocol_name, strlen(protocol_name));

    /* reserved characters */
    for (ii=0;ii<8;ii++)
        bitstream_write_byte((char**)&ptr, 0);

    /* infohash */
    bitstream_write_string((char**)&ptr, expected_ih, 20);

    /* peerid */
    bitstream_write_string((char**)&ptr, my_pi, 20);

    /* calculate total handshake size */
    size = 1 + strlen(protocol_name) + 8 + 20 + 20;

    if (0 == send(callee, udata, buf, size))
    {
//        __log(me, "send,handshake,fail");
        return 0;
    }

    return 1;
}

void* pwp_handshaker_new(char* expected_info_hash, char* mypeerid)
{
    pwp_handshaker_t* me;

    me = calloc(1,sizeof(pwp_handshaker_t));
    me->expected_ih = expected_info_hash;
    me->my_pi = mypeerid;
    return me;
}

void pwp_handshaker_release(void* hs)
{
    pwp_handshaker_t* me = hs;

    free(me);
}

pwp_handshake_t* pwp_handshaker_get_handshake(void* me_)
{
    pwp_handshaker_t* me = me_;

#if 0
    if (me->status == 1)
        return &me->hs;
    return NULL;
#endif
    return &me->hs;
}

char __readbyte(unsigned int* bytes_read, const char **buf, unsigned int* len)
{
    char val;

    val = **buf;
    *buf += 1;
    *bytes_read += 1;
    *len -= 1;
    return val;
}

int pwp_handshaker_dispatch_from_buffer(void* me_, const char** buf, unsigned int* len)
{
    pwp_handshaker_t* me = me_;
    pwp_handshake_t* hs = &me->hs;

    while (0 < *len)
    {

    /* protcol name length
     * The unsigned value of the first byte indicates the length of a
     * character string containing the prot name. In BTP/1.0 this number
     * is 19. The local peer knows its own prot name and hence also the
     * length of it. If this length is different than the value of this
     * first byte, then the connection MUST be dropped. */
        if (me->curr_value == NULL)
        {
            hs->pn_len = __readbyte(&me->bytes_read, buf, len);
            if (0 == hs->pn_len)
            {
                printf("ERROR: invalid length\n");
                return -1;
            }

            me->cur = me->curr_value = hs->pn = malloc(hs->pn_len);
        }
    /* protocol name
    This is a character string which MUST contain the exact name of the 
    prot in ASCII and have the same length as given in the Name Length
    field. The prot name is used to identify to the local peer which
    version of BTP the remote peer uses. If this string is different
    from the local peers own prot name, then the connection is to be
    dropped. */
        else if (me->curr_value == hs->pn)
        {
            *me->cur = __readbyte(&me->bytes_read, buf, len);
            me->cur++;

            /* validate */
            if (me->cur - me->curr_value == hs->pn_len)
            {
                if (0 != strncmp((char*)hs->pn, PROTOCOL_NAME,
                    hs->pn_len < strlen(PROTOCOL_NAME) ?
                        hs->pn_len : strlen(PROTOCOL_NAME)))
                {
                    printf("ERROR: incorrect protocol name\n");
                    return -1;
                }

                me->cur = me->curr_value = hs->reserved = malloc(8);
            }
        }
    /* Reserved The next 8 bytes in the string are reserved for future
     * extensions and should be read without interpretation. */

    /* bep_0005, DHT: TODO
     * Peers supporting the DHT set the last bit of the 8-byte reserved flags
     *  exchanged in the BitTorrent protocol handshake. Peer receiving a
     *  handshake indicating the remote peer supports the DHT should send a
     *  PORT message. It begins with byte 0x09 and has a two byte payload
     *  containing the UDP port of the DHT node in network byte order. Peers
     *  that receive this message should attempt to ping the node on the
     *  received port and IP address of the remote peer. If a response to the
     *  ping is recieved, the node should attempt to insert the new contact
     *  information into their routing table according to the usual rules.  */

        else if (me->curr_value == hs->reserved)
        {
            *(me->cur++) = __readbyte(&me->bytes_read, buf, len);

            /* don't know what to do with set reserved bytes */
            if (*(me->cur-1) != 0)
            {
                //printf("ERROR: unreserved bytes used\n");
//                return -1;
            }

            if (me->cur - me->curr_value == 8)
            {
                me->cur = me->curr_value = hs->infohash = malloc(20);
            }
        }
    /* Info Hash:
    The next 20 bytes in the string are to be interpreted as a 20-byte SHA1
    of the info key in the metainfo file. Presumably, since both the local
    and the remote peer contacted the tracker as a result of reading in the
    same .torrent file, the local peer will recognize the info hash value and
    will be able to serve the remote peer. If this is not the case, then the
    connection MUST be dropped. This situation can arise if the local peer
    decides to no longer serve the file in question for some reason. The info
    hash may be used to enable the client to serve multiple torrents on the
    same port. */
        else if (me->curr_value == hs->infohash)
        {
            *(me->cur++) = __readbyte(&me->bytes_read, buf, len);

            /* validate */
            if (me->cur - me->curr_value == 20)
            {
                /* check info hash matches expected */
                if (0 != strncmp(
                            (char*)hs->infohash,
                            (char*)me->expected_ih, 20))
                {
                    printf("ERROR invalid infohash: '%s' vs '%s'\n",
                            hs->infohash, me->expected_ih);
                    return -1;
                }

                me->cur = me->curr_value = hs->peerid = malloc(20);
            }
        }
    /* Peer ID:
    The last 20 bytes of the handshake are to be interpreted as the
    self-designated name of the peer. The local peer must use this name to
    identify the connection hereafter. Thus, if this name matches the local
    peers own ID name, the connection MUST be dropped. Also, if any other
    peer has already identified itself to the local peer using that same peer
    ID, the connection MUST be dropped. */
        else if (me->curr_value == hs->peerid)
        {
            *(me->cur++) = __readbyte(&me->bytes_read, buf, len);

            if (me->cur - me->curr_value == 20)
            {
#if 0
                /* disconnect if peer's ID is the same as ours */
                if (!strncmp(peer_id,me->my_pi,20))
                {
                    __disconnect(me, "handshake: peer_id same as ours (us: %s them: %.*s)",
                            me->my_pi, 20, peer_id);
                    return 0;
                }
#endif
//                me->status = 1;
                return 1;//me->bytes_read;
            }
        }
        else
        {
            printf("ERROR: invalid handshake\n");
            return -1;
        }
    }

    return 0;//me->bytes_read;
}

