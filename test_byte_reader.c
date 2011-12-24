#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

void TestByteReaderReadZero(
    CuTest * tc
)
{
    char num[10];

    char *cptr_read;

    unsigned char val;

    memset(num, 0, sizeof(char) * 10);

    cptr_read = &num[0];

    val = stream_read_ubyte(&cptr_read);

    CuAssertTrue(tc, val == 0);
}

void TestByteReaderUbyteWriteRead(
    CuTest * tc
)
{
    char num[10];

    char *cptr_write, *cptr_read;

    unsigned char val;

    memset(num, 0, sizeof(char) * 10);

    cptr_write = &num[0];
    cptr_read = &num[0];

    stream_write_ubyte(&cptr_write, 0x0FF);
    val = stream_read_ubyte(&cptr_read);

    CuAssertTrue(tc, val == 0x0FF);
}

void TestByteReaderUint32WriteRead(
    CuTest * tc
)
{
    char num[10];

    char *cptr_write, *cptr_read;

    uint32_t val;

    memset(num, 0, sizeof(char) * 10);

    cptr_write = &num[0];
    cptr_read = &num[0];

    stream_write_uint32(&cptr_write, 0x0F070301);
    val = stream_read_uint32(&cptr_read);

    CuAssertTrue(tc, val == 0x0F070301);
}

void TestByteReaderMiscWriteRead(
    CuTest * tc
)
{
    char num[10];

    char *cptr_write, *cptr_read;

    unsigned char cval;

    uint32_t ival;

    memset(num, 0, sizeof(char) * 10);

    cptr_write = &num[0];
    cptr_read = &num[0];

    stream_write_ubyte(&cptr_write, 0xFF);
    stream_write_uint32(&cptr_write, 0x0F070301);
    stream_write_ubyte(&cptr_write, 0xFF);

    cval = stream_read_ubyte(&cptr_read);
    CuAssertTrue(tc, cval == 0xFF);
    ival = stream_read_uint32(&cptr_read);
    CuAssertTrue(tc, ival == 0x0F070301);
    cval = stream_read_ubyte(&cptr_read);
    CuAssertTrue(tc, cval == 0xFF);
}
