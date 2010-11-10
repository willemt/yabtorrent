
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

typedef struct
{
    int len;
    const byte *rest;


} bt_pwp_msg_header_t;

void TestPWPMsgGetType(
    CuTest * tc
)
{

    bt_peer_connection_t ps;

//    bt_peerconnect_receive

    byte msg[10];

    stream_write_uint32(&ptr, 1);

    int type;

    type = bt_pwp_msg_get_type(msg);

    CuAssertTrue(tc, 0 == bencode_string_value(&ben, &ren, &len));
}

void TestReceiveChoke(
    CuTest * tc
)
{

    bt_peer_connection_t ps;

//    bt_peerconnect_receive


//    CuAssertTrue(tc, 0 == bencode_string_value(&ben, &ren, &len));
}

#if 0
typedef struct
{
    int type;

    union
    {


    };

} msg_t;
#endif

void TestPWPMsgSetLen(
    CuTest * tc
)
{
    int data[10];

    bt_pwpmsg_set_len(&data, 10);

    CuAssertTrue(tc, 10 == bt_pwpmsg_get_len(&msg));
}

void TestPWPMsgSetType(
    CuTest * tc
)
{
    int data[10];

    bt_pwpmsg_set_type(&data, 5);

    CuAssertTrue(tc, 5 == bt_pwpmsg_get_type(&msg));
}

/*----------------------------------------------------------------------------*/

void TestPWPMsgChokeWrite(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peerconn_msg_prepare_choke(NULL, NULL, data, &len);

    msg = data;

    CuAssertTrue(tc, 1 == stream_read_uint32(&msg));
    CuAssertTrue(tc, 0 == stream_read_ubyte(&msg));
}

void TestPWPMsgChokeMsgChokesPeer(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peer_connection_t *pconn;

    pconn = bt_peerconn_new();

    bt_peerconn_msg_prepare_choke(NULL, NULL, data, &len);

    CuAssertTrue(tc, !bt_peerconn_is_choked(pconn));

    bt_peerconn_process_msg(pconn, data, &len);
    CuAssertTrue(tc, bt_peerconn_is_choked(pconn));
}

/*----------------------------------------------------------------------------*/

void TestPWPMsgUnchokeWrite(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peerconn_msg_prepare_unchoke(NULL, NULL, data, &len);

    msg = data;

    CuAssertTrue(tc, 1 == stream_read_uint32(&msg));
    CuAssertTrue(tc, 1 == stream_read_ubyte(&msg));
}

void TestPWPMsgUnchokeMsgUnchokesPeer(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peer_connection_t *pconn;

    pconn = bt_peerconn_new();

    bt_peerconn_msg_prepare_choke(NULL, NULL, data, &len);

    CuAssertTrue(tc, !bt_peerconn_is_choked(pconn));

    bt_peerconn_process_msg(pconn, data, &len);
    CuAssertTrue(tc, bt_peerconn_is_choked(pconn));

    bt_peerconn_msg_prepare_unchoke(NULL, NULL, data, &len);
    bt_peerconn_process_msg(pconn, data, &len);
    CuAssertTrue(tc, !bt_peerconn_is_choked(pconn));
}

/*----------------------------------------------------------------------------*/

void TestPWPMsgInterestedWrite(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peerconn_msg_prepare_interested(NULL, NULL, data, &len);

    msg = data;

    CuAssertTrue(tc, 1 == stream_read_uint32(&msg));
    CuAssertTrue(tc, 2 == stream_read_ubyte(&msg));
}

void TestPWPMsgInterestMsgInterestsPeer(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peer_connection_t *pconn;

    pconn = bt_peerconn_new();

    bt_peerconn_msg_prepare_interest(NULL, NULL, data, &len);

    CuAssertTrue(tc, !bt_peerconn_is_interested(pconn));

    bt_peerconn_process_msg(pconn, data, &len);
    CuAssertTrue(tc, bt_peerconn_is_interested(pconn));
}

/*----------------------------------------------------------------------------*/

void TestPWPMsgUninterestedWrite(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peerconn_msg_prepare_uninterested(NULL, NULL, data, &len);

    msg = data;

    CuAssertTrue(tc, 1 == stream_read_uint32(&msg));
    CuAssertTrue(tc, 3 == stream_read_ubyte(&msg));
}

void TestPWPMsgUninterestMsgUninterestsPeer(
    CuTest * tc
)
{
    int data[10], len;

    byte *msg;

    bt_peer_connection_t *pconn;

    pconn = bt_peerconn_new();

    bt_peerconn_msg_prepare_interest(NULL, NULL, data, &len);

    CuAssertTrue(tc, !bt_peerconn_is_interested(pconn));

    bt_peerconn_process_msg(pconn, data, &len);
    CuAssertTrue(tc, bt_peerconn_is_interested(pconn));

    bt_peerconn_msg_prepare_uninterest(NULL, NULL, data, &len);
    bt_peerconn_process_msg(pconn, data, &len);
    CuAssertTrue(tc, !bt_peerconn_is_interested(pconn));
}

/*----------------------------------------------------------------------------*/

void TestPWPMsgHaveRead(
    CuTest * tc
)
{
    bt_peer_connection_t pconn;

    int data[10], len;

    byte *msg;

    bt_peerconn_msg_prepare_have(peer, pconn, data, &len, 0);

    msg = data;

    CuAssertTrue(tc, 5 == stream_read_uint32(&msg));
    CuAssertTrue(tc, 4 == stream_read_ubyte(&msg));
    CuAssertTrue(tc, 0 == stream_read_uint32(&msg));
}

void TestPWPMsgHaveWrite(
    CuTest * tc
)
{
    bt_peer_connection_t pconn;

    int data[10], len;

    byte *msg;

    msg = data;

    stream_write_uint32(&msg, 4);
    stream_write_ubyte(&msg, 4);        // have message id
    stream_write_uint32(&msg, 100);

    CuAssertTrue(tc, 0 == bt_pwp_msg_len_get(&msg));
}

void TestPWPMsgBitFieldWrite(
    CuTest * tc
)
{
#if 0
    int data[10], len;

    byte *msg;

    bt_peerconn_msg_prepare_bitfield(NULL, NULL, data, &len);

    msg = data;

    CuAssertTrue(tc, 5 == stream_read_uint32(&msg));
    CuAssertTrue(tc, 5 == stream_read_ubyte(&msg));
#endif
}

void TestPWPMsgBitFieldWrite(
    CuTest * tc
)
{
#if 0
    int data[10], len;

    byte *msg;

    bt_peerconn_msg_prepare_bitfield(NULL, NULL, data, &len);

    msg = data;

    CuAssertTrue(tc, 5 == stream_read_uint32(&msg));
    CuAssertTrue(tc, 5 == stream_read_ubyte(&msg));
#endif
}



void TestReceiveInvalidInfohash(
    CuTest * tc
)
{

}
