

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestBT_Sha1Equal(CuTest*);
extern void TestBT_ClientAddPeer(CuTest*);
extern void TestBT_ClientCantAddPeerTwice(CuTest*);
extern void TestBT_ClientRemovePeer(CuTest*);
extern void TestBT_SetFailed(CuTest*);
extern void TestBT_SetInterval(CuTest*);
extern void TestBT_SetInfoHash(CuTest*);
extern void TestBT_GeneratedPeeridIs20BytesLong(CuTest*);
extern void TestURL2Host_with_suffix(CuTest*);
extern void TestURL2Host_without_suffix(CuTest*);
extern void TestURL2Host_without_http_prefix(CuTest*);
extern void TestURL2Host_with_port(CuTest*);
extern void TestURL2HostPort_with_suffix(CuTest*);
extern void TestURL2HostPort_without_suffix(CuTest*);
extern void TestURL2HostPort_without_http_prefix(CuTest*);
extern void TestBT_AddPiece(CuTest*);
extern void TestBT_AddingFileIncreasesTotalFileSize(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestBT_Sha1Equal);
    SUITE_ADD_TEST(suite, TestBT_ClientAddPeer);
    SUITE_ADD_TEST(suite, TestBT_ClientCantAddPeerTwice);
    SUITE_ADD_TEST(suite, TestBT_ClientRemovePeer);
    SUITE_ADD_TEST(suite, TestBT_SetFailed);
    SUITE_ADD_TEST(suite, TestBT_SetInterval);
    SUITE_ADD_TEST(suite, TestBT_SetInfoHash);
    SUITE_ADD_TEST(suite, TestBT_GeneratedPeeridIs20BytesLong);
    SUITE_ADD_TEST(suite, TestURL2Host_with_suffix);
    SUITE_ADD_TEST(suite, TestURL2Host_without_suffix);
    SUITE_ADD_TEST(suite, TestURL2Host_without_http_prefix);
    SUITE_ADD_TEST(suite, TestURL2Host_with_port);
    SUITE_ADD_TEST(suite, TestURL2HostPort_with_suffix);
    SUITE_ADD_TEST(suite, TestURL2HostPort_without_suffix);
    SUITE_ADD_TEST(suite, TestURL2HostPort_without_http_prefix);
    SUITE_ADD_TEST(suite, TestBT_AddPiece);
    SUITE_ADD_TEST(suite, TestBT_AddingFileIncreasesTotalFileSize);

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

