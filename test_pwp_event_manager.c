
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

void TestPWPEventManagerInit(
    CuTest * tc
)
{
    void *em;

    em = bt_pwp_eventmanager_init();

    CuAssertTrue(tc, em);
}

static int __cb1(
    int msg_len,
    int msg_id,
    byte * msg,
    void *udata
)
{

    return 1;
}

static int __cb2(
    int msg_len,
    int msg_id,
    byte * msg,
    void *duata
)
{

    return 1;
}

void TestPWPEventManagerAddCB(
    CuTest * tc
)
{
    void *em;

    em = bt_pwp_eventmanager_init();

    CuAssertTrue(tc, 0 == bt_pwp_eventmananger_get_num_cb(em));
    CuAssertTrue(tc, 1 == bt_pwp_eventmananger_add_cb(em, __cb1));
    CuAssertTrue(tc, 1 == bt_pwp_eventmananger_get_num_cb(em));
}

void TestPWPEventManagerRead(
    CuTest * tc
)
{
    void *em;

    byte bytes[10];

    em = bt_pwp_eventmanager_init();

    CuAssertTrue(tc, 1 == bt_pwp_eventmananger_read(em, bytes));
}
