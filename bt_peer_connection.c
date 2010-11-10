
typedef struct
{
    /* un/choked, etc */
//    int flags;

    bool am_choking;            // this client is choking the peer
    bool am_interested;         // this client is interested in the peer
    bool peer_choking;          // peer is choking this client
    bool peer_interested;       // peer is interested in this client 

    /* this bitfield indicates which pieces each client has */
    uint32_t *have_bitfield;

    /* what requests we need to respond to */
    queue_t *request_queue;

    //int peerID;
    bt_peer_t *send_peer;

    /* the udata we send to the peer */
    void *send_udata;

    /* the send callback */
    int (
    *send_func
    )   (
    void *udata,
    void *peer,
    void *send_data,
    int len
    );

    /* get the piece */
    int (
    *getpiece
    )   (
    void *udata,
    int piece
    );

} bt_peer_connection_t;

/*----------------------------------------------------------------------------*/

static void __send_to_peer(
    bt_peer_connection_t * pc,
    bt_peer_t * peer,
    void *data,
    int len
)
{
    assert(pc->send_func);

    pc->send_func(pc->udata, peer, data, len);
}

/*----------------------------------------------------------------------------*/

bt_peer_connection_t *bt_peerconn_new(
)
{
    bt_peer_connection_t *pc;

    pc = calloc(1, sizeof(bt_peer_connection_t));

    return pc;
}

#if 0
void bt_peerconn_set_peer_id(
    void *pco,
    char *peer_id
)
{
    bt_peer_connection_t *pc;

    pc = pco;

    pc->peer_id = peer_id;
}
#endif

void bt_peerconn_set_npieces(
    void *pco,
    char *peer_id
)
{
    bt_peer_connection_t *pc;

    pc = pco;

    pc->npieces = npieces;
}

void bt_peerconn_set_getpiece_func(
    void *pco,
    int (*getpiece) (void *udata,
                     int piece)
)
{
    bt_peer_connection_t *pc;

    pc = pco;

    pc->getpiece_func = getpiece;
}

void bt_peerconn_set_send_udata(
    void *pco,
    void *udata
)
{
    bt_peer_connection_t *pc;

    pc = pco;

    pc->send_udata = udata;
}

void bt_peerconn_set_send_func(
    void *pco,
    int (*send) (void *udata,
                 void *peer,
                 void *send_data,
                 int len)
)
{
    bt_peer_connection_t *pc;

    pc = pco;

    pc->send_func = send;
}

/**
 * fit the request in the piece size so that we don't break anything */
static void __request_fit(
    pwp_piece_block_t * request,
    int piece_len
)
{
    if (piece_len < request->block_byte_offset + request->block_len)
    {
        request->block_len =
            request->block_byte_offset + request->block_len - piece_len;
    }
}

/**
 * build the following request block for the peer, from this piece.
 * Assume that we want to complete the piece by going through the piece in 
 * sequential blocks. */
static void __piece_build_request(
    bt_piece_t * pce,
    pwp_piece_block_t * request
)
{
    assert(!__piece_done(pce));

    int offset, len;

    request->piece_idx = pce->idx;

    raprogress_get_incomplete(pce->progress, &offset, &len, BLOCK_SIZE);

    request->block_byte_offset = offset + len;
    request->block_len = len;
}

static bool __piece_done(
    bt_piece_t * pce,
    int piece_size
)
{

    // if sha1 matches properly - then we are done
    // // write to disk / disk write queue

}

static void __piece_do_progress(
    bt_piece_t * pce,
    int offset,
    int len
)
{
}

static void __piece_add_block(
    bt_piece_t * pce,
    int offset,
    int len,
    char **data
)
{
    int ii;

    if (!pce->data)
    {
        pce->data = calloc(bt->pce_len, sizeof(char));
    }

    for (ii = 0; ii < len; ii++)
    {
        byte val;

        val = stream_read_ubyte(&msg);

        memcpy(piece->data + offset + ii, &val, sizeof(byte));
    }

    /* fill up piece */
//    memcpy(piece->data + offset, data, len);

    //__piece_do_progress(pce, offset, len);

    raprogress_mark_complete(pce->progress, offset, len);
}

static bool __peerconn_has_piece(
    int id,
    int peer_id,
    int piece_idx
)
{
    int piece_idx;

    int cint;

    cint = piece_idx / 32;

    return (peerconn->have_bitfield[cint] & (1 << (32 - piece_idx % 32)));
}

static bool __peerconn_mark_piece(
    bt_peer_connection_t * peerconn,
    int piece_idx
)
{
    int piece_idx;

    int cint;

    cint = piece_idx / 32;
    peerconn->have_bitfield[cint] |= 1 << (32 - piece_idx % 32);

    return TRUE;
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
 */
#if 0
static void __receive_pwp(
    byte * msg,
    /* receiving from: */
    bt_peer_t * peer,
    bt_peer_connection_t * peerconn
)
{
    uint32_t msg_len;

    msg_len = stream_read_uint32(&msg);

    /* keep alive message */
    if (0 == msg_len)
    {
        FIXME_STUB;
    }
    /* no payload */
    else if (1 <= msg_len)
    {
        byte msg_id;

        uint32_t payload_len;

        payload_len = msg_len - 1;

        msg_id = stream_read_ubyte(&msg);

        switch (msg_id)
        {
        case PWP_MSGTYPE_CHOKE:
            peerconn->peer_choking = TRUE;
            break;
        case PWP_MSGTYPE_UNCHOKE:
            peerconn->peer_choking = FALSE;
            break;
        case PWP_MSGTYPE_INTERESTED:
            peerconn->peer_interested = TRUE;
            break;
        case PWP_MSGTYPE_UNINTERESTED:
            peerconn->peer_interested = FALSE;
            break;
        case PWP_MSGTYPE_HAVE:
            {
                uint32 piece_idx;

                assert(payload_len == 4);

                piece_idx = stream_read_uint32(&msg);

                __peerconn_mark_piece(peerconn, piece_idx);

                assert(__peerconn_has_piece(peer_id, piece_idx));
            }
            break;
        case PWP_MSGTYPE_BITFIELD:
            /* A peer MUST send this message immediately after the handshake operation, and MAY choose not to send it if it has no pieces at all. This message MUST not be sent at any other time during the communication. */
            {
                int ii, piece_idx;

                piece_idx = 0;

                for (ii = 0; ii < payload_len; ii++)
                {
                    byte cur_byte;

                    int bit;

                    cur_byte = stream_read_ubyte(&msg);

                    for (bit = 0; bit < 8; bit++)
                    {
                        if (((cur_byte << bit) >> 7) & 1)
                        {
                            __peerconn_mark_piece(peerconn, piece_idx);
                        }

                        piece_idx += 1;
                    }
                }
            }
            break;
        case PWP_MSGTYPE_REQUEST:
            {
                pwp_piece_block_t request;

                assert(payload_len == 12);

                request.piece_idx = stream_read_uint32(&msg);
                request.block_byte_offset = stream_read_uint32(&msg);
                request.block_len = stream_read_uint32(&msg);

                if (peerconn->am_choking)
                {
                    return;
                }

                if (!peerconn->peer_interested)
                {
                    return;
                }

                if (__has_piece(bt, request.piece_idx))
                {
                    __pwp_send_piece_to_peer(bt, &request, peer);
                }
                else
                    assert(FALSE);
            }
            break;
/*
6.3.10 Piece

This message has ID 7 and a variable length payload. The payload holds 2 integers indicating from which piece and with what offset the block data in the 3rd member is derived. Note, the data length is implicit and can be calculated by subtracting 9 from the total message length. The payload has the following structure:

-------------------------------------------
| Piece Index | Block Offset | Block Data |
-------------------------------------------
*/
        case PWP_MSGTYPE_PIECE:
            {
                char *block_data;

                pwp_piece_block_t request;

                bt_piece_t *piece;

                request.piece_idx = stream_read_uint32(&msg);
                request.block_byte_offset = stream_read_uint32(&msg);
                request.block_len = msg_len - 9;

                piece = __get_piece(bt, request.piece_idx);

                __piece_add_block(piece,
                                  request.block_byte_offset,
                                  request.block_len, &msg);

                if (__piece_done(piece))
                {
                    __pwp_send_have(bt, peer, peerconn, request.piece_idx);
                }
            }
            break;
        case PWP_MSGTYPE_CANCEL:
            /* ---------------------------------------------
             * | Piece Index | Block Offset | Block Length |
             * ---------------------------------------------*/
            {
                pwp_piece_block_t request;

                assert(payload_len == 12);

                request.piece_idx = stream_read_uint32(&msg);
                request.block_byte_offset = stream_read_uint32(&msg);
                request.block_len = stream_read_uint32(&msg);

                FIXME_STUB;
//                queue_remove(peer->request_queue);
            }
            break;
        }
    }
}
#endif

/**
 * unchoke, choke, interested, uninterested */
static void __pwp_send_statechange(
    bt_peer_connection_t * pc,
    bt_peer_t * peer,
    int msg_type
)
{
    byte data[5], *ptr;

    ptr = data;

    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, msg_type);
    __send_to_peer(pc, peer, data, 1);
}

static bt_piece_t *__get_piece(
    bt_peer_connection_t *,
    const int piece_idx
)
{
    return NULL;
}

static void __pwp_send_piece_to_peer(
    bt_peer_connection_t * pc,
    pwp_piece_block_t * request,
    bt_peer_t * peer
)
{
    byte block[BLOCK_SIZE + 10], *ptr;

    bt_piece_t *piece;

    int ii, size;

    ptr = block;
    size = 9 + request->block_len;

//    assert(__has_piece(bt, request->piece_idx));
    piece = __get_piece(pc, request->piece_idx);
//    assert(__piece_done(piece, bt->piece_len));

    stream_write_uint32(&ptr, size);
    stream_write_ubyte(&ptr, PWP_MSGTYPE_PIECE);
    stream_write_uint32(&ptr, request->piece_idx);
    stream_write_uint32(&ptr, request->block_byte_offset);

    for (ii = 0; ii < request->block_len; ii++)
    {
        stream_write_ubyte(&ptr, piece->data[ii + request->block_byte_offset]);
    }

//    memcpy(block, piece->data + request->block_byte_offset, request->block_len);

    __send_to_peer(pc, peer, block, strlen(data));
}


void bt_pwpmsg_set_len(
    byte * data,
    int value
)
{
    stream_write_uint32(&data, value);
}

int bt_pwpmsg_get_len(
    byte * data,
    int value
)
{
//    stream_write_uint32(&data, value);
    return 0;
}

void bt_pwpmsg_set_type(
    byte * data,
    int value
)
{
    data += 4;
    stream_write_ubyte(&data, value);
}

int bt_pwpmsg_get_type(
    byte * data,
    int value
)
{
//    stream_write_uint32(&data, value);
    return 0;
}

/**
 * @return 0 on error, 1 otherwise */
int bt_peerconn_msg_do_have(
    bt_peer_t * peer,
    bt_peer_connection_t * peerconn,
    byte * data,
    int *len,
    int piece_idx
)
{
/*
Implementer's Note: That is the strict definition, in reality some games may be played. In particular because peers are extremely unlikely to download pieces that they already have, a peer may choose not to advertise having a piece to a peer that already has that piece. At a minimum "HAVE suppression" will result in a 50% reduction in the number of HAVE messages, this translates to around a 25-35% reduction in protocol overhead. At the same time, it may be worthwhile to send a HAVE message to a peer that has that piece already since it will be useful in determining which piece is rare.
*/
#if 0
    if (__peerconn_has_piece(peerconn, piece_idx))
    {
        return 0;
    }
#endif

    //byte data[12], *ptr;
    byte *ptr;

    ptr = data;

    stream_write_uint32(&ptr, 4);
    stream_write_ubyte(&ptr, PWP_MSGTYPE_HAVE);
    stream_write_uint32(&ptr, piece_idx);

    *len = 4;

//    __send_to_peer(bt, peer, data, 4);

    return 1;
}

static void __pwp_send_request(
    bt_peer_connection_t * pc,
    bt_peer_t * peer,
    pwp_piece_block_t * request
)
{
    byte data[12], *ptr;

    ptr = block;

    stream_write_uint32(&ptr, 13);
    stream_write_uint32(&ptr, request->piece_idx);
    stream_write_uint32(&ptr, request->block_byte_offset);
    stream_write_uint32(&ptr, request->block_len);
    __send_to_peer(pc, peer, data, 13);
}

#if 0
static char *__generate_peer_id(
)
{

    FIXME_STUB;

    return NULL;
}

void bt_tc_connect_to_tracker(
    int id,
    /* the sha1 of the metainfo file */
    char *info_hash,
    /* the port we are listening on */
    int port
)
{
    char *string;

    trackerclient_t *tc;

    tc->peer_id = __generate_peer_id();
//    1. The local peer opens a port on which to listen for incoming connections from remote peers
//    2. This port is then reported to the tracker. 
    sprintf(string, "info_hash, peer_id, port");
    http_request(url, string);
//    3. got response
}
#endif

static void __pwp_send_bitfield(
    int id
)
{
#if 0
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

    int ii, cint;

    uint32 *bitfield;

    int size;

    size = (bt->npieces + bt->npieces % 32) / 32;
    bitfield = calloc(size, sizeof(uint32));
    for (cint = 0, ii = 0; ii < bt->npieces; ii++)
    {
//        piece_status_t *piecestat;
//        piecestat = &tc->piece_status[ii];

        if (tc->piece_status[ii].have)
        {
            /* mark as downloaded */
            bitfield[cint] |= 1 << (32 - ii % 32);
        }

        if (ii % 32 == 0 && ii != 0)
        {
//            bitfield[cint] = htonl(bitfield[cint]);
            cint += 1;
        }
    }
#endif

    byte data[1000], *ptr, bits;

    uint32_t size;

    int ii;

    ptr = block;

    size = 1 + (bt->npieces / 8) + ((bt->npieces % 8 == 0) ? 0 : 1);

    stream_write_uint32(&ptr, size);
    stream_write_ubyte(&ptr, PWP_MSGTYPE_BITFIELD);

    for (bits = 0, ii = 0; ii < bt->npieces; ii++)
    {
        bt_piece_t *piece;

        piece = __get_piece(bt, ii);
        bits |= __piece_done(piece) << (7 - (ii % 8));

        if (ii % 8 == 0 && ii != 0)
        {
            stream_write_ubyte(&ptr, bits);
            bits = 0;
        }
    }

    if (ii % 8 != 0)
    {
        stream_write_ubyte(&ptr, bits);
    }

    __send_to_peer(bt, peer, data, size);
}

/*----------------------------------------------------------------------------*/

/**
 *
 *
 * receive a handshake from another peer
 *
 * ----------------------------------------------------------------
 * | Name Length | Protocol Name | Reserved | Info Hash | Peer ID |
 * ----------------------------------------------------------------
 *
 * @return true if sucessful, false if invalid */
static bool __pwp_receive_handshake(
    int id,
    const char *handshake,
    const char *info_hash,
    bt_peer_t * peer_out
)
{
    byte name_len;

    /* other peers name protocol name */
    char peer_pname[strlen(PROTOCOL_NAME)];

    char peer_infohash[INFOHASH_LEN];

    char peer_id[PEERID_LEN];

    name_len = handshake[0];
    // Name Length:
    // The unsigned value of the first byte indicates the length of a character
    // string containing the protocol name. In BTP/1.0 this number is 19. The
    // local peer knows its own protocol name and hence also the length of it.
    // If this length is different than the value of this first byte, then the
    // connection MUST be dropped. 
    if (name_len != strlen(PROTOCOL_NAME))
    {
        return FALSE;
    }
    // Protocol Name:
    // This is a character string which MUST contain the exact name of the 
    // protocol in ASCII and have the same length as given in the Name Length
    // field. The protocol name is used to identify to the local peer which
    // version of BTP the remote peer uses.
    // If this string is different from the local peers own protocol name, then
    // the connection is to be dropped. 
    strncpy(peer_pname, &handshake[1], name_len);
    if (strcmp(peer_pname, PROTOCOL_NAME))
    {
        return FALSE;
    }

    // Reserved:
    // The next 8 bytes in the string are reserved for future extensions and
    // should be read without interpretation. 

    // Info Hash:
    // The next 20 bytes in the string are to be interpreted as a 20-byte SHA1 of the info key in the metainfo file. Presumably, since both the local and the remote peer contacted the tracker as a result of reading in the same .torrent file, the local peer will recognize the info hash value and will be able to serve the remote peer. If this is not the case, then the connection MUST be dropped. This situation can arise if the local peer decides to no longer serve the file in question for some reason. The info hash may be used to enable the client to serve multiple torrents on the same port. 

    strncpy(peer_infohash, &handshake[1 + name_len + 8], INFOHASH_LEN);
    if (strncmp(peer_infohash, info_hash))
    {
        return FALSE;
    }
    // At this stage, if the connection has not been dropped, then the local peer MUST send its own handshake back, which includes the last step: 

    // Peer ID:
    // The last 20 bytes of the handshake are to be interpreted as the self-designated name of the peer. The local peer must use this name to identify the connection hereafter. Thus, if this name matches the local peers own ID name, the connection MUST be dropped. Also, if any other peer has already identified itself to the local peer using that same peer ID, the connection MUST be dropped. 
//    bt_tc_add_peer();

    strncpy(peer_id, &handshake[1 + name_len + 8 + INFOHASH_LEN], PEERID_LEN);
    peer->peed_id = strdup(peer_id);
    peerconn->peer_choking = TRUE;
    peerconn->am_choking = TRUE;
    return TRUE;
}

int bt_peerconn_send_handshake(
    void *pco,
    const char *infohash,
    const char *peer_id
)
{
    byte data[512];

    char *protocol_name = PROTOCOL_NAME;

    sprintf(buf, "%c%s%s%s%s%s",
            strlen(protocol_name), protocol_name,
            PWP_HANDSHAKE_RESERVERD, infohash, peer_id);

    __send_to_peer(pc, pc->peer, data, strlen(data));
}

void bt_peerconn_step(
    void *pco
)
{
    bt_peer_connection_t *pc;

    pc = pco;

    if (pc->peer_interested)
    {
        if (pc->am_choking)
        {
            __pwp_send_statechange(pc, pc->peer, PWP_MSGTYPE_UNCHOKE);
        }
    }

    /* download a piece */

#if 0
    int jj;

    for (jj = 0; jj < bt->npieces; jj++)
    {
        bt_piece_t *piece;

        pwp_piece_block_t request;

        piece = __get_piece(pc, jj);

        if (__piece_done(piece))
            continue;

        __piece_build_request(piece, &request);
        __request_fit(request, bt->piece_len);
        __pwp_send_request(bt, pc, &request);
        break;
    }
#endif
}
