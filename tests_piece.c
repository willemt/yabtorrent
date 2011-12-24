

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestBTPiece_new_has_set_size_and_hashsum(CuTest*);
extern void TestBTPiece_set_idx(CuTest*);
extern void TestBTPiece_full_request_means_piece_is_fully_requested(CuTest*);
extern void TestBTPiece_unfull_request_means_piece_is_not_fully_requested(CuTest*);
extern void TestBTPiece_pollBlockRequest_has_default_blockSize(CuTest*);
extern void TestBTPiece_pollBlockRequest_sized_under_threshhold(CuTest*);
extern void TestBTPiece_write_block_needs_disk_blockrw(CuTest*);
extern void TestBTPiece_cannot_read_block_we_dont_have(CuTest*);
extern void TestBTPiece_write_block_means_block_can_be_read(CuTest*);
extern void TestBTPiece_doneness_is_valid(CuTest*);
extern void TestBTPiece_Write_Block_To_Stream(CuTest*);
extern void TestBTPiece_Write_Block_To_Str(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestBTPiece_new_has_set_size_and_hashsum);
    SUITE_ADD_TEST(suite, TestBTPiece_set_idx);
    SUITE_ADD_TEST(suite, TestBTPiece_full_request_means_piece_is_fully_requested);
    SUITE_ADD_TEST(suite, TestBTPiece_unfull_request_means_piece_is_not_fully_requested);
    SUITE_ADD_TEST(suite, TestBTPiece_pollBlockRequest_has_default_blockSize);
    SUITE_ADD_TEST(suite, TestBTPiece_pollBlockRequest_sized_under_threshhold);
    SUITE_ADD_TEST(suite, TestBTPiece_write_block_needs_disk_blockrw);
    SUITE_ADD_TEST(suite, TestBTPiece_cannot_read_block_we_dont_have);
    SUITE_ADD_TEST(suite, TestBTPiece_write_block_means_block_can_be_read);
    SUITE_ADD_TEST(suite, TestBTPiece_doneness_is_valid);
    SUITE_ADD_TEST(suite, TestBTPiece_Write_Block_To_Stream);
    SUITE_ADD_TEST(suite, TestBTPiece_Write_Block_To_Str);

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

