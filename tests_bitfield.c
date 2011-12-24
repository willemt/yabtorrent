

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include "CuTest.h"


extern void TestBTBitfield_get_length_gets_initialised_length(CuTest*);
extern void TestBTBitfield_markbit_is_marked(CuTest*);
extern void TestBTBitfield_unmarkbit_is_unmarked(CuTest*);
extern void TestBTBitfield_str_produces_correct_string(CuTest*);


void RunAllTests(void) 
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();


    SUITE_ADD_TEST(suite, TestBTBitfield_get_length_gets_initialised_length);
    SUITE_ADD_TEST(suite, TestBTBitfield_markbit_is_marked);
    SUITE_ADD_TEST(suite, TestBTBitfield_unmarkbit_is_unmarked);
    SUITE_ADD_TEST(suite, TestBTBitfield_str_produces_correct_string);

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

