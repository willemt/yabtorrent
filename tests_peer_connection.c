

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestPWP_init_has_us_choked(CuTest*);
extern void TestPWP_send_handshake_understands_peer_is_unreachable(CuTest*);
extern void TestPWP_send_state_change_is_wellformed(CuTest*);
extern void TestPWP_send_Have_is_wellformed(CuTest*);
extern void TestPWP_send_BitField_is_wellformed(CuTest*);
extern void TestPWP_send_Request_is_wellformed(CuTest*);
extern void TestPWP_send_Piece_is_wellformed(CuTest*);
extern void TestPWP_send_Cancel_is_wellformed(CuTest*);
extern void TestPWP_choke_sets_as_choked(CuTest*);
extern void TestPWP_unchoke_sets_as_unchoked(CuTest*);
extern void TestPWP_no_reading_without_handshake(CuTest*);
extern void TestPWP_read_HaveMsg_marks_peer_as_having_piece(CuTest*);
extern void TestPWP_read_ChokeMsg_marks_us_as_choked(CuTest*);
extern void TestPWP_read_ChokeMsg_empties_our_pending_requests(CuTest*);
extern void TestPWP_read_UnChokeMsg_marks_us_as_unchoked(CuTest*);
extern void TestPWP_read_PeerIsInterested_marks_peer_as_interested(CuTest*);
extern void TestPWP_read_PeerIsUnInterested_marks_peer_as_uninterested(CuTest*);
extern void TestPWP_read_Bitfield_marks_peers_pieces_as_haved_by_peer(CuTest*);
extern void TestPWP_read_Request_of_piece_not_completed_disconnects_peer(CuTest*);
extern void TestPWP_requesting_block_increments_pending_requests(CuTest*);
extern void TestPWP_read_Piece_decreases_pending_requests(CuTest*);
extern void TestPWP_read_CancelMsg_cancels_last_request(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestPWP_init_has_us_choked);
    SUITE_ADD_TEST(suite, TestPWP_send_handshake_understands_peer_is_unreachable);
    SUITE_ADD_TEST(suite, TestPWP_send_state_change_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_Have_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_BitField_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_Request_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_Piece_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_send_Cancel_is_wellformed);
    SUITE_ADD_TEST(suite, TestPWP_choke_sets_as_choked);
    SUITE_ADD_TEST(suite, TestPWP_unchoke_sets_as_unchoked);
    SUITE_ADD_TEST(suite, TestPWP_no_reading_without_handshake);
    SUITE_ADD_TEST(suite, TestPWP_read_HaveMsg_marks_peer_as_having_piece);
    SUITE_ADD_TEST(suite, TestPWP_read_ChokeMsg_marks_us_as_choked);
    SUITE_ADD_TEST(suite, TestPWP_read_ChokeMsg_empties_our_pending_requests);
    SUITE_ADD_TEST(suite, TestPWP_read_UnChokeMsg_marks_us_as_unchoked);
    SUITE_ADD_TEST(suite, TestPWP_read_PeerIsInterested_marks_peer_as_interested);
    SUITE_ADD_TEST(suite, TestPWP_read_PeerIsUnInterested_marks_peer_as_uninterested);
    SUITE_ADD_TEST(suite, TestPWP_read_Bitfield_marks_peers_pieces_as_haved_by_peer);
    SUITE_ADD_TEST(suite, TestPWP_read_Request_of_piece_not_completed_disconnects_peer);
    SUITE_ADD_TEST(suite, TestPWP_requesting_block_increments_pending_requests);
    SUITE_ADD_TEST(suite, TestPWP_read_Piece_decreases_pending_requests);
    SUITE_ADD_TEST(suite, TestPWP_read_CancelMsg_cancels_last_request);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main(int argc, char **argv)
{
    RunAllTests();
    return 0;
}

