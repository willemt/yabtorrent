

typedef struct
{
    int msg_id;
    int (
    *func
    )   (
    int msg_len,
    int msg_id,
    byte * msg,
    void *udata
    );
} bt_pwp_eventmanager_cb_t;

typedef struct
{
    bt_pwp_eventmanager_cb_t *cbs;
    int cbs_len;
} bt_pwp_eventmanager_t;

int bt_pwp_eventmanager_init(
)
{
    bt_pwp_eventmanager_t *em;

    em = malloc(sizeof(bt_pwp_eventmanager_t));
    memset(em, 0, sizeof(bt_pwp_eventmanager_t));

    return em;
}

int bt_pwp_eventmanager_add_cb(
    bt_pwp_eventmanager_t * em,
    int msg_id,
    int (*func) (int,
                 int,
                 byte *,
                 void *)
)
{
    bt_pwp_eventmanager_t *em;

    if (!em->cbs)
    {

    }

//    em->cbs;

    return 0;
}

int bt_pwp_eventmanager_read_msg(
    void *em,
    byte * msg,
    void *udata
)
{
    bt_pwp_eventmanager_t *em;

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

        __run_cb(em, msg_id);
    }
}

static void __receive_pwp(
    byte * msg,
    /* receiving from: */
    bt_peer_t * peer,
    bt_peer_state_t * peerstate
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
            peerstate->peer_choking = TRUE;
            break;
        case PWP_MSGTYPE_UNCHOKE:
            peerstate->peer_choking = FALSE;
            break;
        case PWP_MSGTYPE_INTERESTED:
            peerstate->peer_interested = TRUE;
            break;
        case PWP_MSGTYPE_UNINTERESTED:
            peerstate->peer_interested = FALSE;
            break;
        case PWP_MSGTYPE_HAVE:
            {
                uint32 piece_idx;

                assert(payload_len == 4);
                piece_idx = stream_read_uint32(&msg);
                __bt_peerstate_mark_piece(peerstate, piece_idx);
                assert(__bt_peerstate_has_piece(peer_id, piece_idx));
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
                            __bt_peerstate_mark_piece(peerstate, piece_idx);
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
                if (peerstate->am_choking)
                {
                    return;
                }

                if (!peerstate->peer_interested)
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
                piece = __bt_get_piece(bt, request.piece_idx);
                __piece_add_block(piece,
                                  request.block_byte_offset,
                                  request.block_len, &msg);
                if (__piece_done(piece))
                {
                    __pwp_send_have(bt, peer, peerstate, request.piece_idx);
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
