

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestBTpiecedb_new_is_empty(CuTest*);
extern void TestBTPieceDB_add_piece(CuTest*);
extern void TestBTPieceDB_GetLength_returns_correct_length_of_db(CuTest*);
extern void TestBTPieceDB_dont_poll_piece_that_peer_doesnt_have(CuTest*);
extern void TestBTPieceDB_poll_best_from_bitfield(CuTest*);
extern void TestBTPieceDB_AddingPiece_LastPieceFitsTotalSize(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestBTpiecedb_new_is_empty);
    SUITE_ADD_TEST(suite, TestBTPieceDB_add_piece);
    SUITE_ADD_TEST(suite, TestBTPieceDB_GetLength_returns_correct_length_of_db);
    SUITE_ADD_TEST(suite, TestBTPieceDB_dont_poll_piece_that_peer_doesnt_have);
    SUITE_ADD_TEST(suite, TestBTPieceDB_poll_best_from_bitfield);
    SUITE_ADD_TEST(suite, TestBTPieceDB_AddingPiece_LastPieceFitsTotalSize);

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

