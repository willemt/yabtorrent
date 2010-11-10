
#include <stdbool.h>
#include "ustr.h"
#include "bencoding.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include "bencode.h"

void TestBencodeWontDoShortExpectedLength(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("4:test");

    const char *ren;

    char *ptr;

    int len;

    ptr = str;
    bencode_init(&ben, str, 3);
    CuAssertTrue(tc, 0 == bencode_string_value(&ben, &ren, &len));
    bencode_done(&ben);
}

void TestBencodeWontDoShortExpectedLength2(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("4:test");

    const char *ren;

    char *ptr;

    int len;

    ptr = str;
    bencode_init(&ben, str, 1);
    CuAssertTrue(tc, 0 == bencode_string_value(&ben, &ren, &len));
    bencode_done(&ben);
}

void TestBencodeIsInt(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("i666e");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_is_int(&ben));
}

void TestBencodeIntValue(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("i666e");

    long int val;

    bencode_init(&ben, str, strlen(str));

    bencode_int_value(&ben, &val);
    CuAssertTrue(tc, 666 == val);
}

void TestBencodeIntValue2(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("i102030e");

    long int val;

    bencode_init(&ben, str, strlen(str));

    bencode_int_value(&ben, &val);
    CuAssertTrue(tc, 102030 == val);
}

void TestBencodeIntValueLarge(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("i2502875232e");

    long int val;

    bencode_init(&ben, str, strlen(str));

    bencode_int_value(&ben, &val);
    CuAssertTrue(tc, 2502875232 == val);
}

void TestBencodeIsIntEmpty(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup(" ");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 0 == bencode_is_int(&ben));
}

void TestBencodeIsString(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("4:test");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_is_string(&ben));
}

void TestBencodeStringValue(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("4:test");

    const char *ren;

    char *ptr;

    int len;

    ptr = str;
    //*ptr = 0;
    bencode_init(&ben, str, strlen(str));
    bencode_string_value(&ben, &ren, &len);
    CuAssertTrue(tc, !strncmp("test", ren, len));
    bencode_done(&ben);
}

void TestBencodeStringValue2(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("12:flyinganimal");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));
    bencode_string_value(&ben, &ren, &len);
    CuAssertTrue(tc, !strncmp("flyinganimal", ren, len));
    bencode_done(&ben);
}

void TestBencodeStringInvalid(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("5:test");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));
    bencode_string_value(&ben, &ren, &len);
    CuAssertTrue(tc, !ren);
    bencode_done(&ben);
}

void TestBencodeIsStringEmpty(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup(" ");

    bencode_init(&ben, str, strlen(str));
    CuAssertTrue(tc, 0 == bencode_is_string(&ben));
}

void TestBencodeIsList(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("l4:test3:fooe");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_is_list(&ben));
}

void TestBencodeIsListEmpty(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup(" ");

    bencode_init(&ben, str, strlen(str));
    CuAssertTrue(tc, 0 == bencode_is_list(&ben));
}

void TestBencodeListHasNext(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("l4:test3:fooe");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_list_has_next(&ben));
}

void TestBencodeListGetNext(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    char *str = strdup("l3:foo3:bare");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_list_get_next(&ben, &ben2));
    bencode_string_value(&ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp("foo", ren, len));
}

void TestBencodeListGetNextTwice(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    char *str = strdup("l4:test3:fooe");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_list_get_next(&ben, &ben2));
    bencode_string_value(&ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp("test", ren, len));

    CuAssertTrue(tc, 1 == bencode_list_get_next(&ben, &ben2));
    bencode_string_value(&ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp("foo", ren, len));

//    printf("%s\n", str);
//    CuAssertTrue(tc, !strcmp("l4:test3:fooe", str));
}

void TestBencodeIsDict(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("d4:test3:fooe");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_is_dict(&ben));
}

void TestBencodeIsDictEmpty(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup(" ");

    bencode_init(&ben, str, strlen(str));
    CuAssertTrue(tc, 0 == bencode_is_dict(&ben));
}

void TestBencodeDictHasNext(
    CuTest * tc
)
{
    bencode_t ben;

    char *str = strdup("l4:test3:fooe");

    bencode_init(&ben, str, strlen(str));

    CuAssertTrue(tc, 1 == bencode_dict_has_next(&ben));
}

void TestBencodeDictGetNext(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    char *str = strdup("d3:foo3:bare");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp("foo", ren, len));
    bencode_string_value(&ben2, &ren, &len);
    CuAssertPtrNotNull(tc, ren);
    CuAssertTrue(tc, !strncmp("bar", ren, len));
}

void TestBencodeDictGetNextTwice(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    char *str = strdup("d4:test3:egg3:foo3:hame");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp(ren, "test", len));
    bencode_string_value(&ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp("egg", ren, len));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp(ren, "foo", len));
    bencode_string_value(&ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp("ham", ren, len));
//    printf("%s\n", str);
//    CuAssertTrue(tc, !strcmp("l4:test3:fooe", str));
}

void TestBencodeDictGetNextInnerList(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    bencode_t ben3;

    char *str = strdup("d3:keyl4:test3:fooee");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp(ren, "key", len));

    bencode_list_get_next(&ben2, &ben3);
    bencode_string_value(&ben3, &ren, &len);
    CuAssertTrue(tc, !strncmp("test", ren, len));

    bencode_list_get_next(&ben2, &ben3);
    bencode_string_value(&ben3, &ren, &len);
    CuAssertTrue(tc, !strncmp("foo", ren, len));

    CuAssertTrue(tc, !bencode_dict_has_next(&ben));
//    printf("%s\n", str);
//    CuAssertTrue(tc, !strcmp("l4:test3:fooe", str));
}

void TestBencodeDictInnerList(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    bencode_t ben3;

    char *str = strdup("d3:keyl4:test3:fooe3:foo3:bare");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp(ren, "key", len));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);
    CuAssertTrue(tc, !strncmp(ren, "foo", len));

    CuAssertTrue(tc, !bencode_dict_has_next(&ben));
//    printf("%s\n", str);
//    CuAssertTrue(tc, !strcmp("l4:test3:fooe", str));
}

void TestBencodeCloneClones(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    char *str = strdup("d3:keyl4:test3:fooe3:foo3:bare");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    bencode_clone(&ben, &ben2);

    CuAssertTrue(tc, !strcmp(ben.str, ben2.str));
    CuAssertTrue(tc, !strcmp(ben.start, ben2.start));
    CuAssertTrue(tc, ben.len == ben2.len);
}

void TestBencodeDictValueAsString(
    CuTest * tc
)
{
#if 0
    bencode_t ben;

    bencode_t ben2;

    char *str = strdup("d3:keyl4:test3:fooe3:foo3:bare");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

//    bencode_dict_get_value_as_string(&ben, &ren, &len);

    CuAssertTrue(tc, len == strlen("l4:test3:fooe"));
    CuAssertTrue(tc, !strncmp(ren, "l4:test3:fooe", len));
#endif
}

void TestBencodeDictValueAsString2(
    CuTest * tc
)
{
#if 0
    bencode_t ben;

    bencode_t ben2;

    char *str = strdup("d4:infod3:keyl4:test3:fooe3:foo3:baree");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);

    bencode_t ben3;

    bencode_dict_get_next(&ben2, &ben3, &ren, &len);

    printf("FFFF%s\n", ren);

    while (bencode_dict_has_next(&ben2))
    {

        bencode_dict_get_next(&ben2, &ben3, &ren, &len);
    }

    printf("BBBB: %s\n", ben2.str);
//    bencode_dict_get_value_as_string(&ben, &ren, &len);

    CuAssertTrue(tc, len == strlen("l4:test3:fooe"));
    CuAssertTrue(tc, !strncmp(ren, "l4:test3:fooe", len));
#endif
}

void TestBencodeDictGetStartAndLen(
    CuTest * tc
)
{
    bencode_t ben;

    bencode_t ben2;

    char *expected = "d3:keyl4:test3:fooe3:foo3:bare";

    char *str = strdup("d4:infod3:keyl4:test3:fooe3:foo3:baree");

    const char *ren;

    int len;

    bencode_init(&ben, str, strlen(str));

    bencode_dict_get_next(&ben, &ben2, &ren, &len);
    bencode_dict_get_start_and_len(&ben2, &ren, &len);

//    printf("FFF: %d '%s'\n", len, ren);
//    printf("%d\n", strlen(expected));

    CuAssertTrue(tc, len == strlen(expected));
    CuAssertTrue(tc, !strncmp(ren, expected, len));
}
