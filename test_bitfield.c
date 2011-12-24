/*
 * =====================================================================================
 *
 *       Filename:  test_bitfield.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  06/06/11 11:32:52
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "bt.h"
#include "bt_local.h"

void TestBTBitfield_get_length_gets_initialised_length(
    CuTest * tc
)
{
    bt_bitfield_t bf;

    bt_bitfield_init(&bf, 40);
    CuAssertTrue(tc, 40 == bt_bitfield_get_length(&bf));
}

void TestBTBitfield_markbit_is_marked(
    CuTest * tc
)
{
    bt_bitfield_t bf;

    bt_bitfield_init(&bf, 40);
    CuAssertTrue(tc, 0 == bt_bitfield_is_marked(&bf, 5));
    bt_bitfield_mark(&bf, 5);
    CuAssertTrue(tc, 1 == bt_bitfield_is_marked(&bf, 5));
}

void TestBTBitfield_unmarkbit_is_unmarked(
    CuTest * tc
)
{
    bt_bitfield_t bf;

    bt_bitfield_init(&bf, 40);
    bt_bitfield_mark(&bf, 5);
    bt_bitfield_unmark(&bf, 5);
    CuAssertTrue(tc, 0 == bt_bitfield_is_marked(&bf, 5));
}

void TestBTBitfield_str_produces_correct_string(
    CuTest * tc
)
{
    bt_bitfield_t bf;

    char *str;

    bt_bitfield_init(&bf, 20);

    str = bt_bitfield_str(&bf);
    CuAssertTrue(tc, 0 == strcmp(str, "00000000000000000000"));
    free(str);

    bt_bitfield_mark(&bf, 5);
    bt_bitfield_mark(&bf, 7);
    str = bt_bitfield_str(&bf);
    CuAssertTrue(tc, 0 == strcmp(str, "00000101000000000000"));
    free(str);
}
