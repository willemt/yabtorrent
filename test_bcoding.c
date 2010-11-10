#include <stdbool.h>
#include "ustr.h"
#include "bencoding.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

void TestBencodeStr(CuTest* tc)
{
    Ustr* str = USTR1(\11, "fuzzy cats");
    Ustr* encoded;

    encoded = bencode_string(str);

    ustr_cmp_cstr_eq(encoded, "10:fuzzy cats");

    CuAssertTrue(tc, 1 == ustr_cmp_cstr_eq(encoded, "10:fuzzy cats"));

    ustr_free(&encoded);
}

void TestBencodeStrZeroSizedStrings(CuTest* tc)
{
    Ustr* str = USTR1(\10, "");
    Ustr* encoded;

    encoded = bencode_string(str);
    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "0:"));

    ustr_free(&encoded);
}

void TestBencodeInt(CuTest* tc)
{
    Ustr* encoded;

    encoded = bencode_int(11);
    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "i11e"));
    ustr_free(&encoded);

    encoded = bencode_int(12);
    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "i12e"));
    ustr_free(&encoded);

    encoded = bencode_int(789);
    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "i789e"));
    ustr_free(&encoded);
}

void TestBencodeIntNegativeNumbers(CuTest* tc)
{
    Ustr* encoded;

    encoded = bencode_int(-42);
    CuAssertTrue(tc, 1==ustr_cmp_cstr_eq(encoded, "i-42e"));
    ustr_free(&encoded);
}

void TestBencodeListEmpty(CuTest* tc)
{
    Ustr* encoded;

    encoded = bencode_list();
    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "le"));
    ustr_free(&encoded);
}

void TestBencodeList(CuTest* tc)
{
    Ustr* str = USTR1(\10, "fuzzy cats");
    Ustr* str2 = USTR1(\10, "green eggs");
    Ustr* list;
    Ustr* stre = bencode_string(str);
    Ustr* stre2 = bencode_string(str2);

    list = bencode_list();
    bencode_list_add(list, stre);
    CuAssertTrue(tc, ustr_cmp_prefix_cstr_eq(list, "l"));
    CuAssertTrue(tc, ustr_cmp_suffix_cstr_eq(list, "e"));
    CuAssertTrue(tc, ustr_len(list) == ustr_len(stre) + 2);
    CuAssertTrue(tc, bencode_list_is_valid(list));

    ustr_free(&list);
}

//    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "l10:fuzzy cats10:green eggse"));
#if 0
void TxestBencodeListIntegers(CuTest* tc)
{
    Ustr* encoded;

    encoded = bencode_list("ii", 42, 13);
    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "li42ei13ee"));
    ustr_free(&encoded);
}

void TxestBencodeListStrings(CuTest* tc)
{
    Ustr* str = USTR1(\10, "fuzzy cats");
    Ustr* str2 = USTR1(\10, "green eggs");
    Ustr* encoded;

    encoded = bencode_list("ss", str, str2);
    CuAssertTrue(tc, ustr_cmp_cstr_eq(encoded, "l10:fuzzy cats5:green eggse"));
    ustr_free(&encoded);
}
#endif

void TestBencodeDict(CuTest* tc)
{
    Ustr* str2 = USTR1(\10, "eggs");
    Ustr* dict;

    Ustr* k1 = USTR1(\10, "cats");
    Ustr* k2 = USTR1(\10, "green");
    Ustr* v1 = USTR1(\10, "");
    Ustr* v2 = USTR1(\10, "green");

    dict = bencode_dict();
    bencode_dict_add(dict, k1, v1);
    bencode_dict_add(dict, k2, v2);
    CuAssertTrue(tc, ustr_cmp_prefix_cstr_eq(dict, "d"));
    CuAssertTrue(tc, ustr_cmp_suffix_cstr_eq(dict, "e"));
    CuAssertTrue(tc, ustr_len(dict) ==
            ustr_len(k1) + ustr_len(k2) + ustr_len(v1) + ustr_len(v2) + 2);
    CuAssertTrue(tc, bencode_dict_is_valid(dict));

    ustr_free(&dict);
}

void TestBencodeDictEmpty(CuTest* tc)
{
    Ustr* dict;

    dict = bencode_dict();
    CuAssertTrue(tc, ustr_cmp_cstr_eq(dict, "de"));
    ustr_free(&dict);
}


