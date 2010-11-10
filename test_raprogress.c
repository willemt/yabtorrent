#include <stdbool.h>
#include "ustr.h"
#include "bencoding.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

void TestRaprogessInit(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, prog != NULL);
}

void TestRaprogressIsNotComplete(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, !raprogress_is_complete(prog));
}

void TestRaprogessMark(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 100);

    CuAssertTrue(tc, raprogress_is_complete(prog));
}

void TestRaprogessMarkAddsBlocks(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, 0 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 0, 10);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 20, 10);

//    printf("%d\n", raprogress_get_num_blocks(prog));

    CuAssertTrue(tc, 2 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 40, 10);

//    printf("%d\n", raprogress_get_num_blocks(prog));

    CuAssertTrue(tc, 3 == raprogress_get_num_blocks(prog));
}

void TestRaprogessMarkHalves(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 50);

    CuAssertTrue(tc, !raprogress_is_complete(prog));

    raprogress_mark_complete(prog, 50, 50);

    CuAssertTrue(tc, raprogress_is_complete(prog));
}

void TestRaprogessMergingLeftHappens(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, 0 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 25, 25);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 0, 25);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));
}

void TestRaprogessMergingRightHappens(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, 0 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 0, 25);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 25, 25);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));
}

void TestRaprogessMergingEaten(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, 0 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 0, 40);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 10, 30);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));
}

void TestRaprogessMergingMiddleHappens(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, 0 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 0, 40);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 60, 40);

    CuAssertTrue(tc, 2 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 40, 20);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));
}

void TestRaprogessMergingHappensAndDoesnt(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, 0 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 0, 25);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 25, 25);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_mark_complete(prog, 51, 25);

    CuAssertTrue(tc, 2 == raprogress_get_num_blocks(prog));
}

void TestRaprogessMarkQuarters(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 25);

    CuAssertTrue(tc, !raprogress_is_complete(prog));

    raprogress_mark_complete(prog, 50, 25);

    CuAssertTrue(tc, !raprogress_is_complete(prog));

    raprogress_mark_complete(prog, 75, 25);

    CuAssertTrue(tc, !raprogress_is_complete(prog));

    raprogress_mark_complete(prog, 25, 25);

    CuAssertTrue(tc, raprogress_is_complete(prog));
}

void TestRaprogessMarkHugeSwallow(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 9);
    raprogress_mark_complete(prog, 10, 9);
    raprogress_mark_complete(prog, 20, 9);
    raprogress_mark_complete(prog, 30, 9);
    raprogress_mark_complete(prog, 40, 9);
    raprogress_mark_complete(prog, 50, 9);
    raprogress_mark_complete(prog, 5, 95);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));
    CuAssertTrue(tc, raprogress_is_complete(prog));
}

void TestRaprogessMark2(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 25, 50);
    raprogress_mark_complete(prog, 0, 10);
    CuAssertTrue(tc, 2 == raprogress_get_num_blocks(prog));
}

void TestRaprogessGetIncomplete(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    int len, offset;

    raprogress_get_incomplete(prog, &offset, &len, 32);

    CuAssertTrue(tc, offset == 0);
    CuAssertTrue(tc, len == 32);
}

void TestRaprogessGetIncomplete2(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    int len, offset;

    raprogress_mark_complete(prog, 0, 50);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_get_incomplete(prog, &offset, &len, 32);

    CuAssertTrue(tc, offset == 50);
    CuAssertTrue(tc, len == 32);
}

void TestRaprogessGetIncomplete3(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    int len, offset;

    raprogress_mark_complete(prog, 25, 50);

    CuAssertTrue(tc, 1 == raprogress_get_num_blocks(prog));

    raprogress_get_incomplete(prog, &offset, &len, 32);

    CuAssertTrue(tc, offset == 0);
    CuAssertTrue(tc, len == 25);
}

void TestRaprogessGetIncomplete4(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    int len, offset;

    raprogress_mark_complete(prog, 25, 50);
    raprogress_mark_complete(prog, 0, 10);

    CuAssertTrue(tc, 2 == raprogress_get_num_blocks(prog));

    raprogress_get_incomplete(prog, &offset, &len, 32);

    CuAssertTrue(tc, offset == 10);
    CuAssertTrue(tc, len == 15);
}
