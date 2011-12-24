#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <arpa/inet.h>

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include "bt.h"
#include "bt_local.h"

typedef struct
{
    int pos;
    char *data;

    char last_send_data[128];
    bt_piece_t *piece;

    char infohash[21];
} test_sender_t;

typedef struct
{
    int pos;
    char *data;
    int has_disconnected;

    bt_piece_t *piece;

    bt_block_t last_block;
    unsigned char last_block_data[10];
    char infohash[21];
} test_reader_t;

static char *__FUNC_get_infohash_reader(
    test_reader_t * reader,
    void *peer
)
{
    return reader->infohash;
}

static char *__FUNC_get_infohash_sender(
    test_sender_t * reader,
    void *peer
)
{
    return reader->infohash;
}

static int __FUNC_peercon_recv(
    test_reader_t * reader,
    bt_peer_t * peer,
    char *buf,
    int *len
)
{
    memcpy(buf, &reader->data[reader->pos], sizeof(char) * (*len));
    reader->pos += *len;

#if 0
    printf("read:");
    int ii;

    for (ii = 0; ii < *len; ii++)
    {
        printf("%d ", buf[ii]);
    }
    printf("\n");
#endif

    return 1;
}

static void *__FUNC_reader_get_piece(
    test_reader_t * reader,
    const int idx
)
{
    bt_piece_t *piece;

    piece = reader->piece;
    return piece;
}

/*----------------------------------------------------------------------------*/
bt_piece_info_t __pinfo = {
    .piece_len = 20,
    .npieces = 20
};

static void __FUNC_disconnect(
    test_reader_t * reader,
    bt_peer_t * peer,
    char *reason
)
{
    reader->has_disconnected = 1;
}

static void __FUNC_MOCK_push_block(
    test_reader_t * reader,
    bt_peer_t * peer,
    bt_block_t * block,
    void *data
)
{

}

/*  allocate the block */
static void __FUNC_push_block(
    test_reader_t * reader,
    bt_peer_t * peer,
    bt_block_t * block,
    void *data
)
{
    memcpy(&reader->last_block, block, sizeof(bt_block_t));
    assert(block->block_len < 10);
    memcpy(reader->last_block_data, data, sizeof(char) * block->block_len);
}

static void *__reader_set(
    test_reader_t * reader,
    char *msg
)
{
    reader->pos = 0;
    reader->data = msg;
    reader->has_disconnected = 0;

    /*  add the piece db to reader */
    bt_block_t blk;
    char piecedata[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    void *dc;

    dc = bt_diskmem_new();
    bt_diskmem_set_size(dc, 4);
    reader->piece = bt_piece_new("00000000000000000000", 4);
    blk.block_byte_offset = 0;
    blk.block_len = 3;
    bt_piece_set_disk_blockrw(reader->piece, bt_diskmem_get_blockrw(dc), dc);
    bt_piece_write_block(reader->piece, NULL, &blk, piecedata);

    return msg;
}

/*----------------------------------------------------------------------------*/

void TestPWP_init_has_us_choked(
    CuTest * tc
)
{
    void *pc;

    pc = bt_peerconn_new();
    CuAssertTrue(tc, 1 == bt_peerconn_im_choked(pc));
}

/*----------------------------------------------------------------------------*/
/*  Send data                                                                 */
/*----------------------------------------------------------------------------*/

static char *__sender_set(
    test_sender_t * sender
)
{
    bt_block_t blk;
    char piecedata[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    void *dc;

    dc = bt_diskmem_new();
    bt_diskmem_set_size(dc, 4);

    memset(sender, 0, sizeof(test_sender_t));
    sender->piece = bt_piece_new("00000000000000000000", 4);
    /* write block of length 4 */
    /* piece idx not necessary */
    blk.block_byte_offset = 0;
    blk.block_len = 4;
    bt_piece_set_disk_blockrw(sender->piece, bt_diskmem_get_blockrw(dc), dc);
    bt_piece_write_block(sender->piece, NULL, &blk, piecedata);
    return sender->last_send_data;
}

/*  Just a mock.
 *  Used when sent data is not tested */
static int __FUNC_MOCK_send(
    test_sender_t * sender,
    const void *peer,
    const unsigned char *send_data,
    const int len
)
{
    return 1;
}

static int __FUNC_send(
    test_sender_t * sender,
    const void *peer,
    const unsigned char *send_data,
    const int len
)
{
    memcpy(sender->last_send_data + sender->pos, send_data, len);

#if 0
    int ii;

    for (ii = 0; ii < len; ii++)
        printf("%02x,", send_data[ii]);
    printf("\n");
#endif

    sender->pos += len;

    return 1;
}

static int __FUNC_failing_send(
    test_sender_t * sender,
    const void *peer,
    const unsigned char *send_data,
    const int len
)
{
    return 0;
}

static void *__FUNC_sender_get_piece(
    test_sender_t * sender,
    const int idx
)
{
    bt_piece_t *piece;

    piece = sender->piece;
//    bt_piece_set_idx(piece, idx);
    return piece;
}

/*
 * Mock an always complete piece */
static int __FUNC_pieceiscomplete(
    void *bto,
    void *piece
)
{
//    bt_piece_t *piece;
//    piece = sender->piece;
    return 1;
}

/*
 * this function pretends as every piece is incomplete
 */
static int __FUNC_pieceiscomplete_fail(
    void *bto,
    void *piece
)
{
    return 0;
}

/*----------------------------------------------------------------------------*/

/*
 * Set the HandshakeSent state when we send the handshake
 */
void TestPWP_send_handshake_sets_handshake_sent_state(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    bt_peerconn_set_peer_id(pc, "00000000000000000000");
    /*  handshaking requires infohash */
    strcpy(sender.infohash, "00000000000000000000");
    bt_peerconn_set_func_get_infohash(pc, (void *) __FUNC_get_infohash_sender);
    bt_peerconn_send_handshake(pc);
    CuAssertTrue(tc, (PC_HANDSHAKE_SENT & bt_peerconn_get_state(pc)));
}

/* *
 * Don't set handshake sent state when the send fails
 * */
void TestPWP_handshake_sent_state_not_set_when_send_failed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    /*  this sending function fails on every send */
    bt_peerconn_set_func_send(pc, (void *) __FUNC_failing_send);
    bt_peerconn_set_peer_id(pc, "00000000000000000000");
    /*  handshaking requires infohash */
    strcpy(sender.infohash, "00000000000000000000");
    bt_peerconn_set_func_get_infohash(pc, (void *) __FUNC_get_infohash_sender);
    bt_peerconn_send_handshake(pc);
    CuAssertTrue(tc, 0 == (PC_HANDSHAKE_SENT & bt_peerconn_get_state(pc)));
}

/*
 * A request message spawns an appropriate response
 * Note: request message needs to check if we completed the requested piece
 * TODO: ADD SEND FUNCTIONALITY TO MOCK OBJECT
 */
void TestPWP_send_Request_spawns_wellformed_response(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte msg[10], *ptr = msg;

    bt_block_t request;

    request.piece_idx = 1;
    request.block_byte_offset = 0;
    request.block_len = 2;

    /*  piece */
    ptr = __sender_set(&sender);        //, msg);
#if 0
    stream_write_uint32(&ptr, 12);
    stream_write_ubyte(&ptr, 6);        /* request */
    stream_write_uint32(&ptr, 1);       /*  piece one */
    stream_write_uint32(&ptr, 0);       /*  block offset 0 */
    stream_write_uint32(&ptr, 2);       /*  block length 2 */
#endif
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED | PC_BITFIELD_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_push_block(pc, (void *) __FUNC_push_block);
    bt_peerconn_set_func_getpiece(pc, (void *) __FUNC_sender_get_piece);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    /*  request message needs to check if we completed the requested piece */
    bt_peerconn_set_func_piece_is_complete(pc, (void *) __FUNC_pieceiscomplete);
    bt_peerconn_unchoke(pc);
    bt_peerconn_process_request(pc, &request);
//    bt_peerconn_process_msg(pc);
//    CuAssertTrue(tc, 0 == sender.has_disconnected);
//    CuAssertTrue(tc, 0);
    CuAssertTrue(tc, 8 + 1 + 2 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, PWP_MSGTYPE_PIECE == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 1 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 0 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 0xff == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 0xff == stream_read_ubyte(&ptr));
}

void TestPWP_send_state_change_is_wellformed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte *ptr;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    bt_peerconn_send_statechange(pc, PWP_MSGTYPE_INTERESTED);

    CuAssertTrue(tc, 1 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, PWP_MSGTYPE_INTERESTED == stream_read_ubyte(&ptr));
}


/*
 * HAVE message has payload of 4
 * HAVE message has correct type flag
 */
void TestPWP_send_Have_is_wellformed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte *ptr;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_getpiece(pc, (void *) __FUNC_sender_get_piece);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    bt_peerconn_send_have(pc, 17);

    CuAssertTrue(tc, 5 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, PWP_MSGTYPE_HAVE == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 17 == stream_read_uint32(&ptr));
}

void TestPWP_send_BitField_is_wellformed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte *ptr;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    /*
     * .piece_len = 20,
     * .npieces = 20
     */
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_getpiece(pc, (void *) __FUNC_sender_get_piece);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    /*  piece complete func will always return 1 (ie. piece is complete) */
    bt_peerconn_set_func_piece_is_complete(pc, (void *) __FUNC_pieceiscomplete);
    bt_peerconn_send_bitfield(pc);

    ptr = sender.last_send_data;
    /*  length */
    CuAssertTrue(tc, 4 == stream_read_uint32(&ptr));
    /*  message type */
    CuAssertTrue(tc, PWP_MSGTYPE_BITFIELD == stream_read_ubyte(&ptr));
    /* 11111111 11111111 11110000  */
    /*  please note the mock get_piece is always returning a piece */
    CuAssertTrue(tc, 0XFF == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 0XFF == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 0XF0 == stream_read_ubyte(&ptr));
}

/*
 * Request message has payload of 6
 */
void TestPWP_send_Request_is_wellformed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte *ptr;

    bt_block_t blk;

    blk.piece_idx = 1;
    blk.block_byte_offset = 0;
    blk.block_len = 20;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    bt_peerconn_send_request(pc, &blk);

    CuAssertTrue(tc, 13 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 6 == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 1 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 0 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 20 == stream_read_uint32(&ptr));
}

#if 0
void TxestPWP_send_RequestForPieceOnlyIfPeerHasPiece_is_wellformed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte *ptr;

    bt_block_t blk;

    blk.piece_idx = 1;
    blk.block_byte_offset = 0;
    blk.block_len = 20;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    bt_peerconn_send_request(pc, &blk);

    bt_peerconn_mark_peer_has_piece(pc, 1);

    CuAssertTrue(tc, 13 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 6 == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 1 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 0 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 20 == stream_read_uint32(&ptr));
}
#endif

void TestPWP_send_Piece_is_wellformed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte *ptr;

    bt_block_t blk;

    /*  get us 4 bytes of data */
    blk.piece_idx = 0;
    blk.block_byte_offset = 0;
    blk.block_len = 4;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_func_getpiece(pc, (void *) __FUNC_sender_get_piece);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    bt_peerconn_send_piece(pc, &blk);
    /*  length */
    CuAssertTrue(tc, 13 == stream_read_uint32(&ptr));
    /*  msgtype */
    CuAssertTrue(tc, 7 == stream_read_ubyte(&ptr));
    /* piece idx  */
    CuAssertTrue(tc, 0 == stream_read_uint32(&ptr));
    /*  block offset */
    CuAssertTrue(tc, 0 == stream_read_uint32(&ptr));
    /*  block payload */
    CuAssertTrue(tc, 0XDE == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 0XAD == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 0XBE == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 0XEF == stream_read_ubyte(&ptr));
}

/*
 * Cancel has payload of 12
 */
void TestPWP_send_Cancel_is_wellformed(
    CuTest * tc
)
{
    void *pc;

    test_sender_t sender;

    byte *ptr;

    bt_block_t blk;

    blk.piece_idx = 1;
    blk.block_byte_offset = 0;
    blk.block_len = 20;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_send);
    bt_peerconn_send_cancel(pc, &blk);

    CuAssertTrue(tc, 13 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 8 == stream_read_ubyte(&ptr));
    CuAssertTrue(tc, 1 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 0 == stream_read_uint32(&ptr));
    CuAssertTrue(tc, 20 == stream_read_uint32(&ptr));
}

/*----------------------------------------------------------------------------*/

/*
 * The peer is recorded as chocked when we set it to choked
 * */
void TestPWP_choke_sets_as_choked(
    CuTest * tc
)
{
    void *pc;

    pc = bt_peerconn_new();
    bt_peerconn_choke(pc);
    CuAssertTrue(tc, bt_peerconn_peer_is_choked(pc));
}

void TestPWP_unchoke_sets_as_unchoked(
    CuTest * tc
)
{
    void *pc;

    pc = bt_peerconn_new();
    bt_peerconn_choke(pc);
    bt_peerconn_unchoke(pc);
    CuAssertTrue(tc, !bt_peerconn_peer_is_choked(pc));
}

/*----------------------------------------------------------------------------*/
/*  Receive data                                                              */
/*----------------------------------------------------------------------------*/

/*
 * Disconnect if non-handshake data arrives before the handshake
 */
void TestPWP_no_reading_without_handshake(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;


    /*  choke */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);       /*  length = 1 */
    stream_write_ubyte(&ptr, 0);        /*  0 = choke */
    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_state(pc, PC_CONNECTED);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_peer_id(pc, "00000000000000000000");
    /*  handshaking requires infohash */
    strcpy(reader.infohash, "00000000000000000000");
    bt_peerconn_set_func_get_infohash(pc, (void *) __FUNC_get_infohash_reader);
    bt_peerconn_process_msg(pc);
    /* we have disconnected */
    CuAssertTrue(tc, 1 == reader.has_disconnected);
//    CuAssertTrue(tc, 0 == bt_peerconn_im_choked(pc));
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
}

/*
 * If bitfield not first message sent after handshake, then disconnect
 */
void TestPWP_read_msg_other_than_bitfield_after_handshake_disconnects(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 5);
    stream_write_ubyte(&ptr, 4);        /*  have */
    stream_write_uint32(&ptr, 1);       /*  piece 1 */

    /*  peer connection */
    pc = bt_peerconn_new();
    /*  current state is with handshake just completed */
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_process_msg(pc);

    CuAssertTrue(tc, 1 == reader.has_disconnected);
}

/*
 * HAVE message marks peer as having this piece
 */
void TestPWP_read_HaveMsg_marks_peer_as_having_piece(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 5);       /*  length */
    stream_write_ubyte(&ptr, 4);        /*  HAVE */
    stream_write_uint32(&ptr, 1);       /*  piece 1 */

    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, bt_peerconn_peer_has_piece(pc, 1));
}

/*
 * A peer receiving a HAVE message with bad index will:
 *  - Drop the connection
 */
void TestPWP_read_havemsg_disconnects_with_piece_idx_out_of_bounds(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 5);
    stream_write_ubyte(&ptr, 4);
    stream_write_uint32(&ptr, 10000);

    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_process_msg(pc);

    CuAssertTrue(tc, 1 == reader.has_disconnected);
}

/*
 * A peer receiving a HAVE message MUST send an interested message to the sender if indeed it lacks the piece announced
 * NOTE: NON-CRITICAL
 */
void TestPWP_read_send_interested_if_lacking_piece_from_have_msg(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0);
}

/*
 * Choke message chokes us
 */
void TestPWP_read_ChokeMsg_marks_us_as_choked(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  choke */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, 0);
    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == bt_peerconn_im_choked(pc));
}

void TestPWP_read_ChokeMsg_empties_our_pending_requests(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    bt_block_t blk;

    /*  block */
    memset(&blk, 0, sizeof(bt_block_t));
    /*  choke */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);       /* length = 1 */
    stream_write_ubyte(&ptr, 0);        /* choke = 0 */
    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_MOCK_send);
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_request_block(pc, &blk);
    CuAssertTrue(tc, 1 == bt_peerconn_get_npending_requests(pc));
    bt_peerconn_process_msg(pc);
    /*  queue is now empty */
    CuAssertTrue(tc, 0 == bt_peerconn_get_npending_requests(pc));
}

/*
 * Unchoke message unchokes peer
 */
void TestPWP_read_UnChokeMsg_marks_us_as_unchoked(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  choke */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, 0);
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == bt_peerconn_im_choked(pc));

    /*  unchoke */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, 1);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 0 == bt_peerconn_im_choked(pc));
}

/*
 * Interested message shows peer is interested
 */
void TestPWP_read_PeerIsInterested_marks_peer_as_interested(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    CuAssertTrue(tc, 0 == bt_peerconn_peer_is_interested(pc));

    /*  show interest */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);       /* length = 1 */
    stream_write_ubyte(&ptr, 2);        /* interested = 2 */
    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_MOCK_send);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == bt_peerconn_peer_is_interested(pc));
}

/*
 * Uninterested message shows peer is uninterested
 */
void TestPWP_read_PeerIsUnInterested_marks_peer_as_uninterested(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  show interest */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, 2);
    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_MOCK_send);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == bt_peerconn_peer_is_interested(pc));

    /*  show uninterest */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, 3);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 0 == bt_peerconn_peer_is_interested(pc));

}

/*
 * Bitfield message marks peer as having these pieces
 */
void TestPWP_read_Bitfield_marks_peers_pieces_as_haved_by_peer(
    CuTest * tc
)
{
#if 1
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  bitfield */
    stream_write_uint32(&ptr, 4);
    stream_write_ubyte(&ptr, 5);        /*  bitpiece */
    stream_write_ubyte(&ptr, 0xff);     /*  11111111 */
    stream_write_ubyte(&ptr, 0xff);     /*  11111111 */
    stream_write_ubyte(&ptr, 0xf0);     /*  11110000 */
    reader.pos = 0;
    reader.data = msg;
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    CuAssertTrue(tc, 0 == bt_peerconn_peer_has_piece(pc, 0));
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == bt_peerconn_peer_has_piece(pc, 0));
    CuAssertTrue(tc, 1 == bt_peerconn_peer_has_piece(pc, 1));
    CuAssertTrue(tc, 1 == bt_peerconn_peer_has_piece(pc, 2));
    CuAssertTrue(tc, 1 == bt_peerconn_peer_has_piece(pc, 3));
#endif
//    CuAssertTrue(tc, 0);
}

/*
 * Disconnect if bitfield sent more than once 
 */
void TestPWP_read_disconnect_if_Bitfield_received_more_than_once(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  bitfield */
    stream_write_uint32(&ptr, 1);
    stream_write_ubyte(&ptr, 5);
    stream_write_ubyte(&ptr, 0xff);
    stream_write_ubyte(&ptr, 0xff);
    stream_write_ubyte(&ptr, 0xf0);
    reader.pos = 0;
    reader.data = msg;
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED | PC_BITFIELD_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == reader.has_disconnected);
}

#if 0
void TxestPWP_readBitfieldGreaterThanNPieces(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  bitfield */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 4);
    stream_write_ubyte(&ptr, 5);
    stream_write_ubyte(&ptr, 0xF0);     // 11110000
    stream_write_ubyte(&ptr, 0x00);     // 00000000
    stream_write_ubyte(&ptr, 0x00);     // 00000000
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 0 == reader.has_disconnected);
    CuAssertTrue(tc, 1 == bt_peerconn_peer_has_piece(pc, 0));
    CuAssertTrue(tc, 1 == bt_peerconn_peer_has_piece(pc, 3));
    CuAssertTrue(tc, 0 == bt_peerconn_peer_has_piece(pc, 4));

    /*  bad bitfield */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 4);
    stream_write_ubyte(&ptr, 5);
    stream_write_ubyte(&ptr, 0xF0);     // 11110000
    stream_write_ubyte(&ptr, 0x00);     // 00000000
    stream_write_ubyte(&ptr, 0x08);     // 00001000
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == reader.has_disconnected);
}

void TxestPWP_readBitfieldLessThanNPieces(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  bitfield */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 3);
    stream_write_ubyte(&ptr, 5);
    stream_write_ubyte(&ptr, 0xF0);     // 11110000
    stream_write_ubyte(&ptr, 0x00);     // 00000000
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == reader.has_disconnected);
}
#endif

/*----------------------------------------------------------------------------*/



/*
 * The peer will disconnect if we request a piece it hasn't completed.
 * We should know better
 * NOTE: request message needs to check if we completed the requested piece
 */
void TestPWP_read_Request_of_piece_not_completed_disconnects_peer(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 12);      /*  payload length */
    stream_write_ubyte(&ptr, 6);        /*  message type */
    /*  invalid piece idx of zero */
    stream_write_uint32(&ptr, 1);       /*  piece idx */
    stream_write_uint32(&ptr, 0);       /*  block offset */
    /*  invalid length of zero */
    stream_write_uint32(&ptr, 2);       /*  block length */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED | PC_BITFIELD_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_getpiece(pc, (void *) __FUNC_reader_get_piece);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    /*  this is critical for this test: */
    bt_peerconn_set_func_piece_is_complete(pc,
                                           (void *)
                                           __FUNC_pieceiscomplete_fail);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == reader.has_disconnected);
}

/*
 * A request must have a piece index that fits the expected bounds
 */
void TestPWP_read_Request_with_invalid_piece_idx_disconnects_peer(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 12);      /*  payload length */
    stream_write_ubyte(&ptr, 6);        /*  message type */
    /*  invalid piece idx of 62 */
    stream_write_uint32(&ptr, 62);      /*  piece idx */
    stream_write_uint32(&ptr, 0);       /*  block offset */
    stream_write_uint32(&ptr, 1);       /*  block length */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED | PC_BITFIELD_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_getpiece(pc, (void *) __FUNC_reader_get_piece);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    /*  we need this for us to know if the request is valid */
    //bt_peerconn_set_func_piece_is_complete(pc, (void *) __FUNC_pieceiscomplete);
    /*  this is required for this test: */
    bt_peerconn_set_func_piece_is_complete(pc, (void *) __FUNC_pieceiscomplete);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == reader.has_disconnected);
}

/*
 * NON-CRITICAL
 */
void TestPWP_read_Request_with_invalid_block_length_disconnects_peer(
    CuTest * tc
)
{
#if 0
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  1. create a valid, connected, and handshaken peer */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED | PC_BITFIELD_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_getpiece(pc, (void *) __FUNC_reader_get_piece);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    /*  we need this for us to know if the request is valid */
    bt_peerconn_set_func_piece_is_complete(pc, (void *) __FUNC_pieceiscomplete);

    /* 2. peer to send a HAVE */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 5);       /*  length */
    stream_write_ubyte(&ptr, 4);        /*  HAVE */
    stream_write_uint32(&ptr, 1);       /*  piece 1 */
    bt_peerconn_process_msg(pc);

    /*  we should have this piece for the test to be valid */
    CuAssertTrue(tc, bt_peerconn_peer_has_piece(pc, 1));

    /* 3. create piece with invalid block length */
    /* piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 12);      /*  payload length */
    stream_write_ubyte(&ptr, 6);        /*  message type : request */
    stream_write_uint32(&ptr, 1);       /*  piece idx */
    stream_write_uint32(&ptr, 0);       /*  block offset */
    /* invalid length of zero */
    stream_write_uint32(&ptr, 100000);  /*  block length */
    bt_peerconn_process_msg(pc);

    /* 4. check we haven't been disconnected */
    CuAssertTrue(tc, 1 == reader.has_disconnected);
#endif
    CuAssertTrue(tc, 0);
}

/*----------------------------------------------------------------------------*/

/*
 * Disconnect if piece message is received without us requesting the piece
 */
void TestPWP_read_piece_results_in_disconnect_if_we_havent_requested_this_piece(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    /*  piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 11);      /*  payload */
    stream_write_ubyte(&ptr, 7);        /*  piece type */
    stream_write_uint32(&ptr, 1);       /*  piece one */
    stream_write_uint32(&ptr, 0);       /*  byte offset is 0 */
    stream_write_ubyte(&ptr, 0xDE);
    stream_write_ubyte(&ptr, 0xAD);
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_push_block(pc, (void *) __FUNC_push_block);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == reader.has_disconnected);
}

void TestPWP_read_piece_results_in_correct_receivable(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = { 0, 0, 0 };

    byte msg[10], *ptr = msg;

    bt_block_t blk;

    /*  piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 11);      /*  payload */
    stream_write_ubyte(&ptr, 7);        /*  piece type */
    stream_write_uint32(&ptr, 1);       /*  piece one */
    stream_write_uint32(&ptr, 0);       /*  byte offset is 0 */
    stream_write_ubyte(&ptr, 0xDE);
    stream_write_ubyte(&ptr, 0xAD);
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED | PC_HANDSHAKE_SENT |
                          PC_HANDSHAKE_RECEIVED | PC_BITFIELD_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_push_block(pc, (void *) __FUNC_push_block);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    /*  make sure we're at least requesting this piece */
    blk.piece_idx = 1;
    blk.block_byte_offset = 0;
    blk.block_len = 2;
    bt_peerconn_request_block(pc, &blk);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 0 == reader.has_disconnected);
    CuAssertTrue(tc, 1 == reader.last_block.piece_idx);
    CuAssertTrue(tc, 0 == reader.last_block.block_byte_offset);
    CuAssertTrue(tc, 2 == reader.last_block.block_len);
    CuAssertTrue(tc, 0xDE == reader.last_block_data[0]);
    CuAssertTrue(tc, 0xAD == reader.last_block_data[1]);
}

void TestPWP_requesting_block_increments_pending_requests(
    CuTest * tc
)
{
    bt_block_t blk;
    void *pc;

    /*  block */
    memset(&blk, 0, sizeof(bt_block_t));
    /* peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_isr_udata(pc, NULL);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_MOCK_send);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    CuAssertTrue(tc, 0 == bt_peerconn_get_npending_requests(pc));
    bt_peerconn_request_block(pc, &blk);
    CuAssertTrue(tc, 1 == bt_peerconn_get_npending_requests(pc));
}

void TestPWP_read_Piece_decreases_pending_requests(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = {
        0, 0, 0
    };
    byte msg[10], *ptr = msg;
    bt_block_t blk;

    memset(&blk, 0, sizeof(bt_block_t));
    blk.piece_idx = 0;
    blk.block_len = 1;
    /*  piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 10);      /* data length = 9 (hdr) + 1 (data) */
    stream_write_ubyte(&ptr, 7);        /* piece msg = 7 */
    stream_write_uint32(&ptr, 0);       /* piece idx */
    stream_write_uint32(&ptr, 0);       /* block offset */
    stream_write_uint32(&ptr, 0);       /* data */
    /*  peer connection */
    pc = bt_peerconn_new();
    bt_peerconn_set_state(pc,
                          PC_CONNECTED |
                          PC_HANDSHAKE_SENT | PC_HANDSHAKE_RECEIVED);
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_push_block(pc, (void *) __FUNC_MOCK_push_block);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_MOCK_send);
    bt_peerconn_request_block(pc, &blk);
    CuAssertTrue(tc, 1 == bt_peerconn_get_npending_requests(pc));
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 0 == bt_peerconn_get_npending_requests(pc));
}

/*  
 * Cancel last message
 * Cancel removes from peer's request list
 * NON-CRITICAL
 */
void TestPWP_read_CancelMsg_cancels_last_request(
    CuTest * tc
)
{
#if 0
    void *pc;

    test_reader_t reader = {
        0, 0, 0
    };
    byte msg[10], *ptr = msg;

    /*  piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 12);
    stream_write_ubyte(&ptr, 6);
    stream_write_uint32(&ptr, 1);
    stream_write_uint32(&ptr, 0);
    stream_write_uint32(&ptr, 2);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_push_block(pc, (void *) __FUNC_push_block);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 0 == reader.has_disconnected);
    CuAssertTrue(tc, 1 == reader.last_block.piece_idx);
    CuAssertTrue(tc, 0 == reader.last_block.block_byte_offset);
    CuAssertTrue(tc, 2 == reader.last_block.block_len);
    CuAssertTrue(tc, 0xDE == reader.last_block_data[0]);
    CuAssertTrue(tc, 0xAD == reader.last_block_data[1]);
#endif
    CuAssertTrue(tc, 0);
}

/*
 * Handshake is 64 bytes long
 */
void TestPWP_read_handshake_is_64bytes(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0);
}

/*
 * All requests are dropped when a client chokes a peer
 */
void TestPWP_request_queue_dropped_when_peer_is_choked(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0);
}

/*
 * Disconnect if a peer sends a data message while being choked
 */
void TestPWP_read_disconnect_if_peer_requests_while_choked(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0);
}

/*
 * If this name matches the local peers own ID name, the connection MUST be dropped
 */
void TestPWP_read_disconnect_if_handshake_shows_peer_with_our_peerid(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0);
}

/*
 * If any other peer has already identified itself to the local peer using the same peer ID then:
 * NOTE: The connection MUST be dropped
 */
void TestPWP_read_disconnect_if_handshake_shows_a_peer_with_same_peerid_as_other(
    CuTest * tc
)
{
    CuAssertTrue(tc, 0);
}

#if 0
void TxestPWP_readPiece_disconnectsIfBlockTooBig(
    CuTest * tc
)
{
    void *pc;

    test_reader_t reader = {
        0, 0, 0
    };
    byte msg[10], *ptr = msg;

    /*  piece */
    ptr = __reader_set(&reader, msg);
    stream_write_uint32(&ptr, 11);
    stream_write_ubyte(&ptr, 7);
    stream_write_uint32(&ptr, 1);
    stream_write_uint32(&ptr, 0);
    stream_write_ubyte(&ptr, 0xDE);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &reader);
    bt_peerconn_set_func_disconnect(pc, (void *) __FUNC_disconnect);
    bt_peerconn_set_func_push_block(pc, (void *) __FUNC_push_block);
    bt_peerconn_set_func_recv(pc, (void *) __FUNC_peercon_recv);
    bt_peerconn_process_msg(pc);
    CuAssertTrue(tc, 1 == reader.has_disconnected);
}
#endif

typedef struct
{
    int len;
    const byte *rest;
} bt_pwp_msg_header_t;

static int __send(
    void *udata,
    bt_peer_t * peer,
    void *send_data,
    const int len
)
{



}

/*----------------------------------------------------------------------------*/

void TestPWP_send_HandShake_is_wellformed(
    CuTest * tc
)
{
    int data[10], len;
    byte *msg;
    void *pc;
    test_sender_t sender;

    __sender_set(&sender);

    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_func_send(pc, __send);
    bt_peerconn_set_isr_udata(pc, &sender);
    /*  handshaking requires infohash */
    strcpy(sender.infohash, "00000000000000000000");
    bt_peerconn_set_func_get_infohash(pc, (void *) __FUNC_get_infohash_sender);
    bt_peerconn_set_peer_id(pc, "00000000000000000000");
    bt_peerconn_send_handshake(pc);
}

void TestPWP_wont_send_unless_receieved_handshake(
    CuTest * tc
)
{
    void *pc;
    test_sender_t sender;
    byte *ptr;

    ptr = __sender_set(&sender);
    pc = bt_peerconn_new();
    bt_peerconn_set_pieceinfo(pc, &__pinfo);
    bt_peerconn_set_isr_udata(pc, &sender);
    bt_peerconn_set_func_send(pc, (void *) __FUNC_failing_send);
    bt_peerconn_set_active(pc, 1);
    /*  handshaking requires infohash */
    strcpy(sender.infohash, "00000000000000000000");
    bt_peerconn_set_func_get_infohash(pc, (void *) __FUNC_get_infohash_sender);
    bt_peerconn_set_peer_id(pc, "00000000000000000000");
    bt_peerconn_send_handshake(pc);
    CuAssertTrue(tc, 0 == bt_peerconn_is_active(pc));
}
