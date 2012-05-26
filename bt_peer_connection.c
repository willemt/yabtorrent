
/**
 * @file
 * @brief Manage a connection with a peer
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

#include <arpa/inet.h>

#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "bt.h"
#include "bt_local.h"
#include "linked_list_hashmap.h"

static const bt_pwp_cfg_t __default_cfg = {.max_pending_requests = 10 };

/*  state */
typedef struct
{
    /* un/choked, etc */
//    int flags;
    bool im_choking;            // this client is choking the peer
    bool im_interested;         // this client is interested in the peer
    bool peer_choking;          // peer is choking this client
    bool peer_interested;       // peer is interested in this client 
//    int hand_shaked;            // have we handshaked with the peer
    /* this bitfield indicates which pieces the peer has */
//    uint32_t *have_bitfield;
    bt_bitfield_t have_bitfield;

    int flags;
    int failed_connections;

} peer_connection_state_t;

typedef struct
{
//    int id;
    int timestamp;
    bt_block_t blk;
//    UT_hash_handle hh;
} request_t;

/*  peer connection */
typedef struct
{
    peer_connection_state_t state;

    /* what requests we need to respond to */
//    queue_t *request_queue;

//    int piece_interested_in;
    /*  requests that we are waiting to get */
    hashmap_t *pendreqs;

    int isactive;

    bt_piece_info_t *pi;

    //int peerID;
    bt_peer_t *send_peer;

    /* the udata that we use to do sendreceiver work */
    void *isr_udata;

    sendreceiver_i isr;

    func_getpiece_f func_getpiece;
    func_pollblock_f func_pollblock;
    func_have_f func_have;
    func_push_block_f func_push_block;
    func_log_f func_log;
    func_get_infohash_f func_get_infohash;
    func_get_int_f func_piece_is_complete;

    /* how we read and write to blocks */
    bt_blockrw_i *piece_blockrw;

    const char *peer_id;

    const bt_pwp_cfg_t *cfg;

//    const char *infohash;

//    int failed;

} bt_peer_connection_t;

/*----------------------------------------------------------------------------*/

static unsigned long __request_hash(const void *obj)
{
    const bt_block_t *req = obj;

    return req->piece_idx + req->block_len + req->block_byte_offset;
}

static long __request_compare(const void *obj, const void *other)
{
    const bt_block_t *req1 = obj, *req2 = other;

    if (req1->piece_idx == req2->piece_idx &&
        req1->block_len == req2->block_len &&
        req1->block_byte_offset == req2->block_byte_offset)
        return 0;

    return 1;
}

/*----------------------------------------------------------------------------*/

static void __log(bt_peer_connection_t * pc, const char *format, ...)
{
    char buffer[1000];

    va_list args;


    va_start(args, format);
    vsprintf(buffer, format, args);
    if (!pc->func_log)
        return;
    pc->func_log(pc->isr_udata, pc->send_peer, buffer);
}

/*----------------------------------------------------------------------------*/
static void __disconnect(bt_peer_connection_t * pc, const char *reason, ...)
{
    char buffer[128];

    va_list args;

    va_start(args, reason);
    vsprintf(buffer, reason, args);
    if (pc->isr.disconnect)
    {
//        printf("disconnect: %s\n", reason);
        pc->isr.disconnect(pc->isr_udata, pc->send_peer, buffer);
    }
}

static int __read_byte_from_peer(bt_peer_connection_t * pc, byte * val)
{
    unsigned char buf[1000], *ptr = buf;

    int len = 1;

    int read;

    read = pc->isr.recv(pc->isr_udata, pc->send_peer, ptr, &len);

#if 0
    if (0 == pc->isr.recv(pc->isr_udata, pc->send_peer, ptr, &len))
    {
        assert(FALSE);
        return 0;
    }
#endif

    *val = stream_read_ubyte(&ptr);
//    printf("read len:%d b 0x%1x %c\n", read, *val, *val);

    return 1;
}

static int __read_uint32_from_peer(bt_peer_connection_t * pc, uint32_t * val)
{
    unsigned char buf[1000], *ptr = buf;

    int len = 4;

    if (0 == pc->isr.recv(pc->isr_udata, pc->send_peer, ptr, &len))
    {
        assert(FALSE);
        return 0;
    }

    *val = stream_read_uint32(&ptr);
//    printf("read u %d\n", *val);
    return 1;
}


/*----------------------------------------------------------------------------*/
void bt_peerconn_set_active(void *pco, int opt)
{
    bt_peer_connection_t *pc = pco;

    pc->isactive = opt;
}


static int __send_to_peer(bt_peer_connection_t * pc, void *data, const int len)
{
    int ret;

    if (pc->isr.send)
    {
        ret = pc->isr.send(pc->isr_udata, pc->send_peer, data, len);

        if (0 == ret)
        {
            __disconnect(pc, "peer dropped connection\n");
//            bt_peerconn_set_active(pc, 0);
            return 0;
        }
    }

    return 1;
}

/*----------------------------------------------------------------------------*/

void *bt_peerconn_new()
{
    bt_peer_connection_t *pc;

    pc = calloc(1, sizeof(bt_peer_connection_t));
    pc->state.im_interested = FALSE;
    pc->state.im_choking = TRUE;
    pc->state.peer_choking = TRUE;
    pc->state.flags = PC_NONE;
    pc->pendreqs = hashmap_new(__request_hash, __request_compare);
    pc->cfg = &__default_cfg;
//    pc->failed = FALSE;
    return pc;
}

bt_peer_t *bt_peerconn_get_peer(void *pco)
{
    bt_peer_connection_t *pc = pco;

    return pc->send_peer;
}

/**
 * let the caller know if this peerconnection is working. */
int bt_peerconn_is_active(void *pco)
{
    bt_peer_connection_t *pc = pco;

    return pc->isactive;
}

void bt_peerconn_set_pieceinfo(void *pco, bt_piece_info_t * pi)
{
    bt_peer_connection_t *pc = pco;

    pc->pi = pi;
//    assert(0 < pc->pi->npieces);
    bt_bitfield_init(&pc->state.have_bitfield, pc->pi->npieces);
}

void bt_peerconn_set_peer_id(void *pco, const char *peer_id)
{
    bt_peer_connection_t *pc = pco;

    pc->peer_id = strdup(peer_id);
}

void bt_peerconn_set_pwp_cfg(void *pco, bt_pwp_cfg_t * cfg)
{
    bt_peer_connection_t *pc = pco;

    pc->cfg = cfg;
}

void bt_peerconn_set_func_get_infohash(void *pco, func_get_infohash_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->func_get_infohash = func;
}

void bt_peerconn_set_func_send(void *pco, func_send_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->isr.send = func;
}

void bt_peerconn_set_func_getpiece(void *pco, func_getpiece_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->func_getpiece = func;
}

void bt_peerconn_set_func_pollblock(void *pco, func_pollblock_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->func_pollblock = func;
}

void bt_peerconn_set_func_have(void *pco, func_have_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->func_have = func;
}

void bt_peerconn_set_func_connect(void *pco, func_connect_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->isr.connect = func;
}

void bt_peerconn_set_func_disconnect(void *pco, func_disconnect_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->isr.disconnect = func;
}

/*
 * We have just downloaded the block and want to allocate it.  */
void bt_peerconn_set_func_push_block(void *pco, func_push_block_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->func_push_block = func;
}

void bt_peerconn_set_func_recv(void *pco, func_recv_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->isr.recv = func;
}

void bt_peerconn_set_func_log(void *pco, func_log_f func)
{
    bt_peer_connection_t *pc = pco;

    pc->func_log = func;
}

void bt_peerconn_set_func_piece_is_complete(void *pco,
                                            func_get_int_f
                                            func_piece_is_complete)
{
    bt_peer_connection_t *pc = pco;

    pc->func_piece_is_complete = func_piece_is_complete;
}

/*----------------------------------------------------------------------------*/

void bt_peerconn_set_isr_udata(void *pco, void *udata)
{
    bt_peer_connection_t *pc = pco;

    pc->isr_udata = udata;
}

void bt_peerconn_set_peer(void *pco, bt_peer_t * peer)
{

    bt_peer_connection_t *pc = pco;

    pc->send_peer = peer;
}

/*----------------------------------------------------------------------------*/
int bt_peerconn_peer_is_interested(void *pco)
{
    bt_peer_connection_t *pc = pco;

    return pc->state.peer_interested;
}

int bt_peerconn_peer_is_choked(void *pco)
{
    bt_peer_connection_t *pc = pco;

    return pc->state.im_choking;
}

/*
 * @return whether I am choked or not
 */
int bt_peerconn_im_choked(void *pco)
{
    bt_peer_connection_t *pc = pco;

    return pc->state.peer_choking;
}

int bt_peerconn_im_interested(void *pco)
{
    bt_peer_connection_t *pc = pco;

    return pc->state.im_interested;
}

void bt_peerconn_choke(bt_peer_connection_t * pc)
{
    pc->state.im_choking = TRUE;
    bt_peerconn_send_statechange(pc, PWP_MSGTYPE_CHOKE);

}

void bt_peerconn_unchoke(bt_peer_connection_t * pc)
{
    pc->state.im_choking = FALSE;
    bt_peerconn_send_statechange(pc, PWP_MSGTYPE_UNCHOKE);
}

/*----------------------------------------------------------------------------*/

static bt_piece_t *__get_piece(bt_peer_connection_t * pc, const int piece_idx)
{
    assert(pc->func_getpiece);
    return pc->func_getpiece(pc->isr_udata, piece_idx);
}

/*----------------------------------------------------------------------------*/

int bt_peerconn_get_download_rate(const bt_peer_connection_t * pc)
{
    return 0;
}

int bt_peerconn_get_upload_rate(const bt_peer_connection_t * pc)
{
    return 0;
}

/*----------------------------------------------------------------------------*/

/**
 * unchoke, choke, interested, uninterested,
 * @return non-zero if unsucessful */
int bt_peerconn_send_statechange(bt_peer_connection_t * pc, const int msg_type)
{
    byte data[5], *ptr = data;

    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, msg_type);
    __log(pc, "send,%s", bt_pwp_msgtype_to_string(msg_type));
    if (!__send_to_peer(pc, data, 5))
    {
        return 0;
    }

    return 1;
}

/**
 * Send the piece highlighted by this request.
 * @pararm req - the requesting block
 * */
void bt_peerconn_send_piece(void *pco, bt_block_t * req)
{
#define BYTES_SENT 1

    bt_peer_connection_t *pc = pco;
    static int data_len;
    static byte *data = NULL;
    byte *ptr;
    bt_piece_t *pce;
    int ii, size;

    assert(pc);

    /*  get data to send */
    pce = __get_piece(pc, req->piece_idx);
    size = 4 * 3 + 1 + req->block_len;
    if (data_len < size)
    {
        data = realloc(data, size);
        data_len = size;
    }

    ptr = data;
//    assert(__has_piece(bt, req->piece_idx));
//    assert(__piece_is_complete(pce, bt->piece_len));
    stream_write_uint32(&ptr, size - 4);
    stream_write_ubyte(&ptr, PWP_MSGTYPE_PIECE);
    stream_write_uint32(&ptr, req->piece_idx);
    stream_write_uint32(&ptr, req->block_byte_offset);
    bt_piece_write_block_to_stream(pce, req, &ptr);
    __send_to_peer(pc, data, size);
#if 0
    for (ii = req->block_len; ii > 0;)
    {
        int len = BYTES_SENT < ii ? BYTES_SENT : ii;

        bt_piece_write_block_to_str(pce,
                                    req->block_byte_offset +
                                    req->block_len - ii, len, block);
        __send_to_peer(pc, block, len);
        ii -= len;
    }

#endif

    __log(pc, "send,piece,pieceidx=%d block_byte_offset=%d block_len=%d",
          req->piece_idx, req->block_byte_offset, req->block_len);
}

/**
 * @return 0 on error, 1 otherwise */
int bt_peerconn_send_have(void *pco, const int piece_idx)
{
    bt_peer_connection_t *pc = pco;

/*
Implementer's Note: That is the strict definition, in reality some games may be played. In particular because peers are extremely unlikely to download pieces that they already have, a peer may choose not to advertise having a piece to a peer that already has that piece. At a minimum "HAVE suppression" will result in a 50% reduction in the number of HAVE messages, this translates to around a 25-35% reduction in protocol overhead. At the same time, it may be worthwhile to send a HAVE message to a peer that has that piece already since it will be useful in determining which piece is rare.
*/
    byte data[12], *ptr = data;
    bt_piece_t *pce;

    pce = __get_piece(pc, piece_idx);
//    assert(bt_piece_is_complete(pce));
//    assert(bt_piece_is_valid(pce));
    stream_write_uint32(&ptr, 5);
    stream_write_ubyte(&ptr, PWP_MSGTYPE_HAVE);
    stream_write_uint32(&ptr, piece_idx);
    __send_to_peer(pc, data, 9);
    __log(pc, "send,have,pieceidx=%d", piece_idx);
    return 1;
}

void bt_peerconn_send_request(void *pco, const bt_block_t * request)
{
    bt_peer_connection_t *pc = pco;
    byte data[32], *ptr;

    ptr = data;
    stream_write_uint32(&ptr, 13);
    stream_write_ubyte(&ptr, PWP_MSGTYPE_REQUEST);
    stream_write_uint32(&ptr, request->piece_idx);
    stream_write_uint32(&ptr, request->block_byte_offset);
    stream_write_uint32(&ptr, request->block_len);
    __send_to_peer(pc, data, 17);
    __log(pc, "send,request,pieceidx=%d block_byte_offset=%d block_len=%d",
          request->piece_idx, request->block_byte_offset, request->block_len);
}

void bt_peerconn_send_cancel(void *pco, bt_block_t * cancel)
{
    bt_peer_connection_t *pc = pco;
    byte data[32], *ptr;

    ptr = data;
    stream_write_uint32(&ptr, 13);
    stream_write_ubyte(&ptr, PWP_MSGTYPE_CANCEL);
    stream_write_uint32(&ptr, cancel->piece_idx);
    stream_write_uint32(&ptr, cancel->block_byte_offset);
    stream_write_uint32(&ptr, cancel->block_len);
    __send_to_peer(pc, data, 17);
    __log(pc, "send,cancel,pieceidx=%d block_byte_offset=%d block_len=%d",
          cancel->piece_idx, cancel->block_byte_offset, cancel->block_len);
}

static void __write_bitfield_to_stream_from_getpiece_func(byte ** ptr,
                                                          bt_piece_info_t * pi,
                                                          void *udata,
                                                          func_getpiece_f
                                                          func_getpiece,
                                                          func_get_int_f
                                                          func_piece_is_complete)
{
    int ii;
    byte bits;

    assert(func_getpiece);
    assert(func_piece_is_complete);

    /*  for all pieces set bit = 1 if we have that completed piece */
    for (bits = 0, ii = 0; ii < pi->npieces; ii++)
    {
        void *pce;

        pce = func_getpiece(udata, ii);
        bits |= func_piece_is_complete(udata, pce) << (7 - (ii % 8));
        /* ...up to eight bits, write to byte */
        if (((ii + 1) % 8 == 0) || pi->npieces - 1 == ii)
        {
//            printf("writing: %x\n", bits);
            stream_write_ubyte(ptr, bits);
            bits = 0;
        }
    }
}

void bt_peerconn_send_bitfield(void *pco)
{
    bt_peer_connection_t *pc = pco;

    /*
     * Bitfield
     * This message has ID 5 and a variable payload length.The payload is a
     * bitfield representing the pieces that the sender has successfully
     * downloaded,
     * with the high bit in the first byte corresponding to piece index 0. If a
     * bit is cleared it is to be interpreted as a missing piece.A peer MUST
     * send this message immediately after the handshake operation,
     * and MAY choose not to send it if it
     * has no pieces at all.This message MUST not be sent at any other time
     * during the communication.
     */
    byte data[1000], *ptr;
    uint32_t size;
    bt_piece_info_t *pi;

    pi = pc->pi;
    ptr = data;
    size =
        sizeof(uint32_t) + sizeof(byte) + (pi->npieces / 8) +
        ((pi->npieces % 8 == 0) ? 0 : 1);
    stream_write_uint32(&ptr, size - sizeof(uint32_t));
    stream_write_ubyte(&ptr, PWP_MSGTYPE_BITFIELD);
    __write_bitfield_to_stream_from_getpiece_func(&ptr, pc->pi, pc->isr_udata,
                                                  pc->func_getpiece,
                                                  pc->func_piece_is_complete);
#if 0
    /*  ensure padding */
    if (ii % 8 != 0)
    {
//        stream_write_ubyte(&ptr, bits);
    }
#endif

    __send_to_peer(pc, data, size);
    __log(pc, "send,bitfield");
}

/*----------------------------------------------------------------------------*/

#if 0
static int __recv_handshake(bt_peer_connection_t * pc, const char *info_hash)
{
}
#endif
 /****f* bt.peerconnection/recv_handshake*
 * FUNCTION
 *  receive handshake from other end
 *  disconnect on any errors
 * RESULT
 *  1 on success;
 *  otherwise 0
 *
 ******/
int bt_peerconn_recv_handshake(void *pco, const char *info_hash)
{
    bt_peer_connection_t *pc = pco;

    int ii;
    byte name_len;

    /* other peers name protocol name */
    char peer_pname[strlen(PROTOCOL_NAME)];
    char peer_infohash[INFO_HASH_LEN];
    char peer_id[PEER_ID_LEN];
    char peer_reserved[8 + 1];

    // Name Length:
    // The unsigned value of the first byte indicates the length of a character
    // string containing the protocol name. In BTP/1.0 this number is 19. The
    // local peer knows its own protocol name and hence also the length of it.
    // If this length is different than the value of this first byte, then the
    // connection MUST be dropped. 
    if (0 == __read_byte_from_peer(pc, &name_len))
    {
        __disconnect(pc, "handshake: invalid name length: '%d'\n", name_len);
        return 0;
    }

    if (name_len != strlen(PROTOCOL_NAME))
    {
        __disconnect(pc, "handshake: invalid protocol name length: '%d'\n",
                     name_len);
        return FALSE;
    }

    // Protocol Name:
    // This is a character string which MUST contain the exact name of the 
    // protocol in ASCII and have the same length as given in the Name Length
    // field. The protocol name is used to identify to the local peer which
    // version of BTP the remote peer uses.
    // If this string is different from the local peers own protocol name, then
    // the connection is to be dropped. 
    for (ii = 0; ii < name_len; ii++)
    {
        if (0 == __read_byte_from_peer(pc, &peer_pname[ii]))
        {
            __disconnect(pc, "handshake: invalid protocol name char\n");
            return 0;
        }
    }
//    strncpy(peer_pname, &handshake[1], name_len);
    if (strncmp(peer_pname, PROTOCOL_NAME, name_len))
    {
        __disconnect(pc, "handshake: invalid protocol name: '%s'\n",
                     peer_pname);
        return FALSE;
    }

    // Reserved:
    // The next 8 bytes in the string are reserved for future extensions and
    // should be read without interpretation. 
    for (ii = 0; ii < 8; ii++)
    {
        if (0 == __read_byte_from_peer(pc, &peer_reserved[ii]))
        {
            __disconnect(pc, "handshake: reserved bytes not empty\n");
            return 0;
        }
    }
    // Info Hash:
    // The next 20 bytes in the string are to be interpreted as a 20-byte SHA1 of the info key in the metainfo file. Presumably, since both the local and the remote peer contacted the tracker as a result of reading in the same .torrent file, the local peer will recognize the info hash value and will be able to serve the remote peer. If this is not the case, then the connection MUST be dropped. This situation can arise if the local peer decides to no longer serve the file in question for some reason. The info hash may be used to enable the client to serve multiple torrents on the same port. 
    for (ii = 0; ii < INFO_HASH_LEN; ii++)
    {
        if (0 == __read_byte_from_peer(pc, &peer_infohash[ii]))
        {
            __disconnect(pc, "handshake: infohash bytes not empty\n");
            return 0;
        }
    }
//    strncpy(peer_infohash, &handshake[1 + name_len + 8], INFO_HASH_LEN);
    if (strncmp(peer_infohash, info_hash, 20))
    {
        printf("handshake: invalid infohash: '%s' vs '%s'\n", info_hash,
               peer_infohash);
        return FALSE;
    }

    // At this stage, if the connection has not been dropped, then the local peer MUST send its own handshake back, which includes the last step: 

    // Peer ID:
    // The last 20 bytes of the handshake are to be interpreted as the self-designated name of the peer. The local peer must use this name to identify the connection hereafter. Thus, if this name matches the local peers own ID name, the connection MUST be dropped. Also, if any other peer has already identified itself to the local peer using that same peer ID, the connection MUST be dropped. 
    for (ii = 0; ii < PEER_ID_LEN; ii++)
    {
        if (0 == __read_byte_from_peer(pc, &peer_id[ii]))
        {
            __disconnect(pc, "handshake: peer_id length invalid\n");
            return 0;
        }
    }
//    strncpy(peer_id, &handshake[1 + name_len + 8 + INFO_HASH_LEN], PEER_ID_LEN);
    pc->send_peer->peer_id = strdup(peer_id);
//    pc->state.peer_choking = FALSE;
//    pc->state.im_choking = FALSE;

    pc->state.flags |= PC_HANDSHAKE_RECEIVED;
    __log(pc, "[connection],gothandshake,%.*s", 20, pc->peer_id);
    return TRUE;
}

/*
 * 1. send handshake
 * 2. receive handshake
 * 3. show interest
 *
 * @return 0 on failure; 1 otherwise
 */
int bt_peerconn_send_handshake(void *pco)
{
    bt_peer_connection_t *pc = pco;
    byte buf[1024];
    char *protocol_name = PROTOCOL_NAME, *ptr;


    const char *infohash, *peerid;

    assert(pc->func_get_infohash);
    assert(pc->peer_id);

    peerid = pc->peer_id;
    infohash = pc->func_get_infohash(pc->isr_udata, pc->send_peer);

    assert(infohash);
    assert(peerid);
//    sprintf(buf, "%c%s" PWP_PC_HANDSHAKE_RESERVERD "%s%s",
//            strlen(protocol_name), protocol_name, infohash, peerid);
    ptr = buf;
    ptr[0] = strlen(protocol_name);
    ptr += 1;
    strcpy(ptr, protocol_name);
    ptr += strlen(protocol_name);
    memset(ptr, 0, 8);
    ptr += 8;
    memcpy(ptr, infohash, 20);
    ptr += 20;
    memcpy(ptr, peerid, 20);
    ptr += 20;
    int ii, size;

    size = 1 + strlen(protocol_name) + 8 + 20 + 20;
#if 0
    for (ii = 0; ii < size; ii++)
        printf("%c", buf[ii]);
    printf("\n");
#endif
    __log(pc, "send,handshake");
    if (0 == __send_to_peer(pc, buf, size))
    {
        __log(pc, "send,handshake,fail");
        bt_peerconn_set_active(pc, 0);
        return 0;
    }

    pc->state.flags |= PC_HANDSHAKE_SENT;
    return 1;
}

void bt_peerconn_set_state(void *pco, const int state)
{
    bt_peer_connection_t *pc = pco;

    pc->state.flags = state;
}

int bt_peerconn_get_state(void *pco)
{
    bt_peer_connection_t *pc = pco;

    return pc->state.flags;
}

/**
 * mark the peer's bitfield as having this piece. */
int bt_peerconn_mark_peer_has_piece(void *pco, const int piece_idx)
{
    bt_peer_connection_t *pc = pco;

    int bf_len;

    bf_len = bt_bitfield_get_length(&pc->state.have_bitfield);

    if (bf_len <= piece_idx || piece_idx < 0)
    {
        __disconnect(pc, "piece idx fits outside of boundary");
        return 0;
    }

    bt_bitfield_mark(&pc->state.have_bitfield, piece_idx);
    return 1;
}

/*----------------------------------------------------------------------------*/

int bt_peerconn_process_request(bt_peer_connection_t * pc, bt_block_t * request)
{
    bt_piece_t *pce;

    /*  ensure we have correct piece_idx */
    if (pc->pi->npieces < request->piece_idx)
    {
        __disconnect(pc, "requested piece %d has invalid idx",
                     request->piece_idx);
        return 0;
    }

    /*  ensure that we have completed this piece */
    if (!(pce = __get_piece(pc, request->piece_idx)))
    {
        __disconnect(pc, "requested piece %d is not available",
                     request->piece_idx);
        return 0;
    }

    assert(pc->func_piece_is_complete);

    if (0 == pc->func_piece_is_complete(pc, pce))
    {
        __disconnect(pc, "requested piece %d is not completed",
                     request->piece_idx);
        return 0;
    }

//    assert(__piece_is_complete(pce, bt->piece_len));

    if (pc->state.im_choking)
    {
        return 0;
    }

//                if (bt_peerconn_peer_has_piece(pc, request.piece_idx))
    bt_peerconn_send_piece(pc, request);
    return 1;
}

static int __recv_request(bt_peer_connection_t * pc,
                          const int msg_len,
                          const int payload_len,
                          int (*fn_read_byte) (bt_peer_connection_t *,
                                               byte *),
                          int (*fn_read_uint32) (bt_peer_connection_t *,
                                                 uint32_t *))
{
    bt_block_t request;

    /*  ensure payload length is correct */
    if (payload_len != 11)
    {
        __disconnect(pc, "invalid payload size for request: %d", payload_len);
        return 0;
    }

    if (0 == fn_read_uint32(pc, &request.piece_idx) ||
//                    0 == fn_read_uint32(pc, &request.block_byte_offset) ||
        0 == fn_read_uint32(pc, &request.block_len))
    {
        return 0;
    }

    __log(pc, "read,request,pieceidx=%d offset=%d length=%d",
          request.piece_idx, request.block_byte_offset, request.block_len);
    return bt_peerconn_process_request(pc, &request);
}

/*
6.3.10 Piece

This message has ID 7 and a variable length payload. The payload holds 2 integers indicating from which piece and with what offset the block data in the 3rd member is derived. Note, the data length is implicit and can be calculated by subtracting 9 from the total message length. The payload has the following structure:

-------------------------------------------
| Piece Index | Block Offset | Block Data |
-------------------------------------------
*/
static int __recv_piece(bt_peer_connection_t * pc,
                        const int msg_len,
                        const int payload_len,
                        int (*fn_read_byte) (bt_peer_connection_t *,
                                             byte *),
                        int (*fn_read_uint32) (bt_peer_connection_t *,
                                               uint32_t *))
{
    int ii;
    unsigned char *block_data;
    bt_block_t request;
    request_t *req;             //, req_key;

    if (0 == fn_read_uint32(pc, &request.piece_idx))
        return 0;
    if (0 == fn_read_uint32(pc, &request.block_byte_offset))
        return 0;
    request.block_len = msg_len - 9;
    req = hashmap_remove(pc->pendreqs, &request);
    if (!req)
    {
        __disconnect(pc,
                     "err: received a block we did not request: %d %d %d\n",
                     request.piece_idx, request.block_byte_offset,
                     request.block_len);
        return 0;
    }

    block_data = malloc(sizeof(char) * request.block_len);
    for (ii = 0; ii < request.block_len; ii++)
    {
        byte val;

        if (0 == fn_read_byte(pc, &block_data[ii]))
            return 0;
    }

    __log(pc, "read,piece,pieceidx=%d offset=%d length=%d",
          request.piece_idx, request.block_byte_offset, request.block_len);
    pc->func_push_block(pc->isr_udata, pc->send_peer, &request, block_data);
//                int cnt = hashmap_count(pc->pendreqs);
//                memcpy(&req_key.blk, &request, sizeof(bt_block_t));
    /*  there should have been a request polled */
    assert(req);
//                assert(cnt - 1 == hashmap_count(pc->pendreqs));
    free(req);
    free(block_data);
#if 0
    if (bt_piece_is_complete(piece))
    {
        bt_peerconn_send_have(pc, request.piece_idx);
    }
#endif

    return 1;
}

static int __recv_bitfield(bt_peer_connection_t * pc,
                           int payload_len,
                           int (*fn_read_byte) (bt_peer_connection_t *, byte *))
{
    int ii, piece_idx;

//    if (pc->state.flags |= PC_BITFIELD_RECEIVED)
    {

    }

    if (payload_len * 8 < pc->pi->npieces)
    {
        __disconnect(pc, "payload length less than npieces");
        return 0;
    }

    piece_idx = 0;
    for (ii = 0; ii < payload_len; ii++)
    {
        byte cur_byte;
        int bit;

        if (0 == fn_read_byte(pc, &cur_byte))
        {
            __disconnect(pc, "bitfield, can't read expected byte");
            return 0;
        }

        for (bit = 0; bit < 8; bit++)
        {
            /*  bit is ON */
            if (((cur_byte << bit) >> 7) & 1)
            {
                if (pc->pi->npieces <= piece_idx)
                {
                    __disconnect(pc, "bitfield has more than expected");
                }

                bt_peerconn_mark_peer_has_piece(pc, piece_idx);
            }

            piece_idx += 1;
        }
    }

    char *str;

    str = bt_bitfield_str(&pc->state.have_bitfield);
    __log(pc, "read,bitfield,%s", str);
    free(str);
    pc->state.flags |= PC_BITFIELD_RECEIVED;
    return 1;
}

/*
 *
 * -----------------------------------------
 * | Message Length | Message ID | Payload |
 * -----------------------------------------
 *
 * All integer members in PWP messages are encoded as a 4-byte big-endian
 * number. Furthermore, all index and offset members in PWP messages are zero-
 * based.
 *
 * @return 1 on sucess; 0 otherwise
 */
static int __process_msg(void *pco,
                         int (*fn_read_uint32) (bt_peer_connection_t *,
                                                uint32_t *),
                         int (*fn_read_byte) (bt_peer_connection_t *, byte *))
{
    bt_peer_connection_t *pc = pco;
    uint32_t msg_len;

    if (!(pc->state.flags & PC_CONNECTED))
    {
        return -1;
    }

    if (!(pc->state.flags & PC_HANDSHAKE_RECEIVED))
    {
        char *ih;

        ih = pc->func_get_infohash(pc->isr_udata, pc->send_peer);
        /*  receive handshake */
        bt_peerconn_recv_handshake(pco, ih);
        /*  send handshake */
        if (!(pc->state.flags & PC_HANDSHAKE_SENT))
        {
            bt_peerconn_send_handshake(pc);     //, ih, pc->peer_id);
        }

        /*  send bitfield */
        if (pc->func_getpiece)
        {
            bt_peerconn_send_bitfield(pc);
            bt_peerconn_send_statechange(pc, PWP_MSGTYPE_INTERESTED);
        }
        return 0;
    }


    if (0 == fn_read_uint32(pc, &msg_len))
    {
        return 0;
    }

    /* keep alive message */
    if (0 == msg_len)
    {
        __log(pc, "read,keep alive");
    }
    /* payload */
    else if (1 <= msg_len)
    {
        byte msg_id;
        uint32_t payload_len;

        payload_len = msg_len - 1;
        if (0 == fn_read_byte(pc, &msg_id))
        {
            return 0;
        }

        /*  make sure bitfield is received after handshake */
        if (!(pc->state.flags & PC_BITFIELD_RECEIVED))
        {
            if (msg_id != PWP_MSGTYPE_BITFIELD)
            {
                __disconnect(pc, "unexpected message; expected bitfield");
            }
            else if (0 == __recv_bitfield(pc, payload_len, fn_read_byte))
            {
                __disconnect(pc, "bad bitfield");
                return 0;
            }

            return 1;
        }

        switch (msg_id)
        {
        case PWP_MSGTYPE_CHOKE:
            {
                pc->state.peer_choking = TRUE;
                __log(pc, "read,choke");
                request_t *req;
                hashmap_iterator_t iter;

                /*  clear pending requests */
                for (hashmap_iterator(pc->pendreqs, &iter);
                     (req = hashmap_iterator_next(pc->pendreqs, &iter));)
                {
                    req = hashmap_remove(pc->pendreqs, req);
                    free(req);
                }
            }
            break;
        case PWP_MSGTYPE_UNCHOKE:
            printf("unchoked: %lx\n", (long unsigned int) pco);
            pc->state.peer_choking = FALSE;
            __log(pc, "read,unchoke");
            break;
        case PWP_MSGTYPE_INTERESTED:
            pc->state.peer_interested = TRUE;
            if (pc->state.im_choking)
            {
                bt_peerconn_unchoke(pc);
            }
            __log(pc, "read,interested");
            break;
        case PWP_MSGTYPE_UNINTERESTED:
            pc->state.peer_interested = FALSE;
            __log(pc, "read,uninterested");
            break;
        case PWP_MSGTYPE_HAVE:
            {
                uint32_t piece_idx;

                assert(payload_len == 4);
                if (0 == fn_read_uint32(pc, &piece_idx))
                    return 0;
                if (1 == bt_peerconn_mark_peer_has_piece(pc, piece_idx))
                {
//                    assert(bt_peerconn_peer_has_piece(pc, piece_idx));
                }
//                bt_bitfield_mark(&pc->state.have_bitfield, piece_idx);
                __log(pc, "read,have,pieceidx=%d", piece_idx);
//                bt_peerconn_send_interested();
            }
            break;
        case PWP_MSGTYPE_BITFIELD:
            /* A peer MUST send this message immediately after the handshake operation, and MAY choose not to send it if it has no pieces at all. This message MUST not be sent at any other time during the communication. */
            __recv_bitfield(pc, payload_len, fn_read_byte);
            break;
        case PWP_MSGTYPE_REQUEST:
            return __recv_request(pc, msg_len, payload_len, fn_read_byte,
                                  fn_read_uint32);
        case PWP_MSGTYPE_PIECE:
            return __recv_piece(pc, msg_len, payload_len, fn_read_byte,
                                fn_read_uint32);
        case PWP_MSGTYPE_CANCEL:
            /* ---------------------------------------------
             * | Piece Index | Block Offset | Block Length |
             * ---------------------------------------------*/
            {
                bt_block_t request;

                assert(payload_len == 12);
                if (0 == fn_read_uint32(pc, &request.piece_idx))
                    return 0;
                if (0 == fn_read_uint32(pc, &request.block_byte_offset))
                    return 0;
                if (0 == fn_read_uint32(pc, &request.block_len))
                    return 0;
                __log(pc, "read,cancel,pieceidx=%d offset=%d length=%d",
                      request.piece_idx, request.block_byte_offset,
                      request.block_len);
                assert(FALSE);
//                FIXME_STUB;
//                queue_remove(peer->request_queue);
            }
            break;
        }
    }

    return 1;
}

/*
 * fit the request in the piece size so that we don't break anything */
static void __request_fit(bt_block_t * request, int piece_len)
{
    if (piece_len < request->block_byte_offset + request->block_len)
    {
        request->block_len =
            request->block_byte_offset + request->block_len - piece_len;
    }
}

/*
 * read current message from receiving end */
void bt_peerconn_process_msg(void *pco)
{
    __process_msg(pco, __read_uint32_from_peer, __read_byte_from_peer);
}

/*----------------------------------------------------------------------------*/

int bt_peerconn_get_npending_requests(bt_peer_connection_t * pc)
{
    return hashmap_count(pc->pendreqs);
}

void bt_peerconn_request_block(bt_peer_connection_t * pc, bt_block_t * blk)
{
    request_t *req;

//    printf("request block: %d %d %d\n",
//           blk->piece_idx, blk->block_byte_offset, blk->block_len);
    assert(pc);
    assert(pc->pi);
    req = malloc(sizeof(request_t));
    __request_fit(blk, pc->pi->piece_len);
    bt_peerconn_send_request(pc, blk);
    memcpy(&req->blk, blk, sizeof(bt_block_t));
    hashmap_put(pc->pendreqs, &req->blk, req);
}

static void __make_request(bt_peer_connection_t * pc)
{
    int timestamp;
    bt_block_t blk;

    if (0 == pc->func_pollblock(pc->isr_udata, &pc->state.have_bitfield, &blk))
    {
        bt_peerconn_request_block(pc, &blk);
    }
}

void bt_peerconn_step(void *pco)
{
    bt_peer_connection_t *pc;

    pc = pco;

    /*  if the peer is not connected and is contactable */
    if (!(pc->state.flags & PC_CONNECTED) &&
        !(pc->state.flags & PC_UNCONTACTABLE_PEER))
    {
        int ret;
        char *ih;

        assert(pc->isr.connect);

        /*  connect to this peer  */
        __log(pc, "[connecting],%.*s", 20, pc->peer_id);
        ret = pc->isr.connect(pc->isr_udata, pc, pc->send_peer);

        /*  check if we haven't failed before too many times
         *  we do not want to stay in an end-less loop */
        if (0 == ret)
        {
            pc->state.failed_connections += 1;

            if (5 < pc->state.failed_connections)
            {
                pc->state.flags = PC_UNCONTACTABLE_PEER;
            }
            assert(0);
        }
        else
        {
            __log(pc, "[connected],%.*s", 20, pc->peer_id);
            pc->state.flags = PC_CONNECTED;

            assert(pc->func_get_infohash);

            ih = pc->func_get_infohash(pc->isr_udata, pc->send_peer);

            /* send handshake */
            if (!(pc->state.flags & PC_HANDSHAKE_SENT))
            {
                bt_peerconn_send_handshake(pc);
            }

            bt_peerconn_recv_handshake(pc, ih);
        }

        return;
    }

    /*  unchoke interested peer */
    if (pc->state.peer_interested)
    {
        if (pc->state.im_choking)
        {
            bt_peerconn_unchoke(pc);
        }
    }

    /*  request piece */
    if (bt_peerconn_im_interested(pc))
    {
        int ii, end;

//        bt_piece_t *piece;
//        bt_block_t request;

        if (bt_peerconn_im_choked(pc))
        {
//            printf("peer is choking us %lx\n", (long unsigned int) pco);
            return;
        }

        /*  max out pipeline */
        end = pc->cfg->max_pending_requests -
            bt_peerconn_get_npending_requests(pc);

        for (ii = 0; ii < end; ii++)
        {
            __make_request(pc);
        }

//        if (-1 == pc->piece_interested_in)
//        {
//            pc->pollblock_func(pc->isr_udata, pc->state.have_bitfield, &pc->piece_interested_in, );
//        }
//        piece = __get_piece(pc, pc->piece_interested_in);
//        if (__piece_is_complete(piece))
//            continue;
//        assert(piece);
    }
    else
    {
        bt_peerconn_send_statechange(pc, PWP_MSGTYPE_INTERESTED);
        pc->state.im_interested = TRUE;
    }
}

/*----------------------------------------------------------------------------*/

/** 
 *  @return 1 if the peer has this piece; otherwise 0
 */
int bt_peerconn_peer_has_piece(void *pco, const int piece_idx)
{
    bt_peer_connection_t *pc = pco;

#if 0
    printf("read: %lx\n", pc->state.have_bitfield[cint]);
    printf("zead: %lx\n",
           pc->state.have_bitfield[cint] & (1 << (31 - piece_idx % 32)));
#endif
    return bt_bitfield_is_marked(&pc->state.have_bitfield, piece_idx);
}
