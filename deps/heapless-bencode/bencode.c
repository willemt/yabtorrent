
/**
 * Copyright (c) 2014, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @brief Read bencoded data
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "bencode.h"

/**
 * Carry length over to a new bencode object.
 * This is done so that we don't exhaust the buffer */
static int __carry_length(
    bencode_t * be,
    const char *pos
)
{
    assert(0 < be->len);
    return be->len - (pos - be->str);
}

/**
 * @param end The point that we read out to
 * @param val Output of number represented by string 
 * @return 0 if error; otherwise 1 */
static long int __read_string_int(
    const char *sp,
    const char **end,
    long int *val
)
{
    *val = 0;

    if (!isdigit(*sp))
        return 0;

    /* work out number */
    do
    {
        *val *= 10;
        *val += *sp - '0';
        sp++;
    }
    while (isdigit(*sp));

    *end = sp;
    return 1;
}

int bencode_is_dict(
    const bencode_t * be
)
{
    return be->str && *be->str == 'd';
}

int bencode_is_int(
    const bencode_t * be
)
{
    return be->str && *be->str == 'i';
}

int bencode_is_list(
    const bencode_t * be
)
{
    return be->str && *be->str == 'l';
}

int bencode_is_string(
    const bencode_t * be
)
{
    const char *sp;

    sp = be->str;

    assert(sp);

    if (!isdigit(*sp))
        return 0;

    do sp++;
    while (isdigit(*sp));

    return *sp == ':';
}

/**
 * Move to next item
 * @param sp The bencode string we are processing
 * @return Pointer to string on success, otherwise NULL */
static const char *__iterate_to_next_string_pos(
    bencode_t * be,
    const char *sp
)
{
    bencode_t iter;

    bencode_init(&iter, sp, __carry_length(be, sp));

    if (bencode_is_dict(&iter))
    {
        /* navigate to the end of the dictionary */
        while (bencode_dict_has_next(&iter))
        {
            /* ERROR: input string is invalid */
            if (0 == bencode_dict_get_next(&iter, NULL, NULL, NULL))
                return NULL;
        }

        return iter.str + 1;
    }
    else if (bencode_is_list(&iter))
    {
        /* navigate to the end of the list */
        while (bencode_list_has_next(&iter))
        {
            /* ERROR: input string is invalid */
            if (-1 == bencode_list_get_next(&iter, NULL))
                return NULL;
        }

        return iter.str + 1;
    }
    else if (bencode_is_string(&iter))
    {
        int len;
        const char *str;

        /* ERROR: input string is invalid */
        if (0 == bencode_string_value(&iter, &str, &len))
            return NULL;

        return str + len;
    }
    else if (bencode_is_int(&iter))
    {
        const char *end;
        long int val;

        if (0 == __read_string_int(&iter.str[1], &end, &val))
            return NULL;

        assert(end[0] == 'e');

        return end + 1;
    }

    /* input string is invalid */
    return NULL;
}

static const char *__read_string_len(
    const char *sp,
    int *slen
)
{
    *slen = 0;

    if (!isdigit(*sp))
        return NULL;

    do
    {
        *slen *= 10;
        *slen += *sp - '0';
        sp++;
    }
    while (isdigit(*sp));

    assert(*sp == ':');
    assert(0 <= *slen);

    return sp + 1;
}

void bencode_init(
    bencode_t * be,
    const char *str,
    const int len
)
{
    memset(be, 0, sizeof(bencode_t));
    be->str = be->start = str;
    be->str = str;
    be->len = len;
    assert(0 < be->len);
}

int bencode_int_value(
    bencode_t * be,
    long int *val
)
{
    const char *end;

    if (0 == __read_string_int(&be->str[1], &end, val))
        return 0;

    assert(end[0] == 'e');

    return 1;
}

int bencode_dict_has_next(
    bencode_t * be
)
{
    const char *sp = be->str;

    assert(be);

    if (!sp
        /* at end of dict */
        || *sp == 'e'
        /* at end of string */
        || *sp == '\0'
        || *sp == '\r'
        /* at the end of the input string */
        || be->str >= be->start + be->len - 1)
    {
        return 0;
    }

    return 1;
}

int bencode_dict_get_next(
    bencode_t * be,
    bencode_t * be_item,
    const char **key,
    int *klen
)
{
    const char *sp = be->str;
    const char *keyin;
    int len;

    assert(*sp != 'e');

    /* if at start increment to 1st key */
    if (*sp == 'd')
    {
        sp++;
    }

    /* can't get the next item if we are at the end of the dict */
    if (*sp == 'e')
    {
        return 0;
    }

    /* 1. find out what the key's length is */
    keyin = __read_string_len(sp, &len);

    /* 2. if we have a value bencode, lets put the value inside */
    if (be_item)
    {
        *klen = len;
        bencode_init(be_item, keyin + len, __carry_length(be, keyin + len));
    }

    /* 3. iterate to next dict key, or move to next item in parent */
    if (!(be->str = __iterate_to_next_string_pos(be, keyin + len)))
    {
        /*  if there isn't anything else or we are at the end of the string */
        return 0;
    }

#if 0
    /*  if at the end of bencode, check that the 'e' terminator is there */
    if (be->str == be->start + be->len - 1 && *be->str != 'e')
    {
        be->str = NULL;
        return 0;
    }
#endif

    assert(be->str);

    if (key)
    {
        *key = keyin;
    }

    return 1;
}

int bencode_string_value(
    bencode_t * be,
    const char **str,
    int *slen
)
{
    const char *sp;

    *slen = 0;
    
    assert(bencode_is_string(be));
    
    sp = __read_string_len(be->str, slen);
    
    assert(sp);
    assert(0 < be->len);
    
    /*  make sure we still fit within the buffer */
    if (sp + *slen > be->start + (long int) be->len)
    {
        *str = NULL;
        return 0;
    }

    *str = sp;
    return 1;
}

int bencode_list_has_next(
    bencode_t * be
)
{
    const char *sp;

    sp = be->str;
    
    /* empty list */
    if (*sp == 'l' &&
        sp == be->start &&
        *(sp + 1) == 'e')
    {
        be->str++;
        return 0;
    }

    /* end of list */
    if (*sp == 'e')
    {
        return 0;
    }

    return 1;
}

int bencode_list_get_next(
    bencode_t * be,
    bencode_t * be_item
)
{
    const char *sp;

    sp = be->str;

#if 0 /* debugging */
    printf("%.*s\n", be->len - (be->str - be->start), be->str);
#endif

    /* we're at the end */
    if (!sp || *sp == 'e')
        return 0;

    if (*sp == 'l')
    {
        /* just move off the start of this list */
        if (be->start == be->str)
        {
            sp++;
        }
    }

    /* can't get the next item if we are at the end of the list */
    if (*sp == 'e')
    {
        be->str = sp;
        return 0;
    }

    /* populate the be_item if it is available */
    if (be_item)
    {
        bencode_init(be_item, sp, __carry_length(be, sp));
    }

    /* iterate to next value */
    if (!(be->str = __iterate_to_next_string_pos(be, sp)))
    {
        return -1;
    }

    return 1;
}

void bencode_clone(
    bencode_t * be,
    bencode_t * output
)
{
    memcpy(output, be, sizeof(bencode_t));
}

int bencode_dict_get_start_and_len(
    bencode_t * be,
    const char **start,
    int *len
)
{
    bencode_t ben, ben2;
    const char *ren;
    int tmplen;

    bencode_clone(be, &ben);
    *start = ben.str;
    while (bencode_dict_has_next(&ben))
        bencode_dict_get_next(&ben, &ben2, &ren, &tmplen);

    *len = ben.str - *start + 1;
    return 0;
}
