#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include "raprogress.h"

void TestRaprogress_Init(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, prog != NULL);
    raprogress_free(prog);
}

void TestRaprogress_IsNotComplete(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, !raprogress_is_complete(prog));
    raprogress_free(prog);
}

void TestRaprogress_Mark(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 100);

    CuAssertTrue(tc, raprogress_is_complete(prog));
    raprogress_free(prog);
}

void TestRaprogress_MarkAddsBlocks(
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
    raprogress_free(prog);
}

void TestRaprogress_MarkHalves(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 50);

    CuAssertTrue(tc, !raprogress_is_complete(prog));

    raprogress_mark_complete(prog, 50, 50);

    CuAssertTrue(tc, raprogress_is_complete(prog));
    raprogress_free(prog);
}

void TestRaprogress_MergingLeftHappens(
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
    raprogress_free(prog);
}

void TestRaprogress_MergingRightHappens(
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
    raprogress_free(prog);
}

void TestRaprogress_MergingEaten(
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
    raprogress_free(prog);
}

void TestRaprogress_MergingMiddleHappens(
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
    raprogress_free(prog);
}

void TestRaprogress_MergingHappensAndDoesnt(
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
    raprogress_free(prog);
}

void TestRaprogress_MarkQuarters(
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
    raprogress_free(prog);
}

void TestRaprogress_MarkHugeSwallow(
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
    raprogress_free(prog);
}

void TestRaprogress_Mark2(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 25, 50);
    raprogress_mark_complete(prog, 0, 10);
    CuAssertTrue(tc, 2 == raprogress_get_num_blocks(prog));
    raprogress_free(prog);
}

void TestRaprogress_GetIncomplete(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    int len, offset;

    raprogress_get_incomplete(prog, &offset, &len, 32);

    CuAssertTrue(tc, offset == 0);
    CuAssertTrue(tc, len == 32);
    raprogress_free(prog);
}

void TestRaprogress_GetIncomplete2(
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
    raprogress_free(prog);
}

void TestRaprogress_GetIncomplete3(
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
    raprogress_free(prog);
}

void TestRaprogress_GetIncomplete4(
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
    raprogress_free(prog);
}

void TestRaprogress_GetNBytesCompleted(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    int len, offset;

    CuAssertTrue(tc, 0 == raprogress_get_nbytes_completed(prog));

    raprogress_mark_complete(prog, 25, 50);
    CuAssertTrue(tc, 50 == raprogress_get_nbytes_completed(prog));

    raprogress_mark_complete(prog, 0, 10);
    CuAssertTrue(tc, 60 == raprogress_get_nbytes_completed(prog));
    raprogress_free(prog);
}

void TestRaprogress_GetIncompleteWithLenUnderBoundary(
    CuTest * tc
)
{
    void *prog = raprogress_init(100);

    int len, offset;

    raprogress_get_incomplete(prog, &offset, &len, 200);
    CuAssertTrue(tc, 100 == len);
    raprogress_free(prog);
}

void TestRaprogress_no_mark_thus_dont_have(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    CuAssertTrue(tc, !raprogress_have(prog, 0, 10));
    raprogress_free(prog);
}

void TestRaprogress_mark_thus_do_have(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 10);
    CuAssertTrue(tc, raprogress_have(prog, 0, 10));
    raprogress_free(prog);
}

void TestRaprogress_mark_thus_do_have_inside(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 100);
    CuAssertTrue(tc, raprogress_have(prog, 0, 10));
    raprogress_free(prog);
}

void TestRaprogress_mark_thus_do_have2(
    CuTest * tc
)
{
    void *prog;

    prog = raprogress_init(100);

    raprogress_mark_complete(prog, 0, 10);
    raprogress_mark_complete(prog, 11, 15);
    raprogress_mark_complete(prog, 26, 10);
    CuAssertTrue(tc, raprogress_have(prog, 15, 10));
    raprogress_free(prog);
}
