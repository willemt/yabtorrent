
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sha1.h"

#include "bt.h"

int bt_sha1_equal(char * s1, char * s2)
{
    return 0 == memcmp(s1, s2, 20);
#if 0
    return ((*s1 + 0) == *(s1 + 0) & *(s2 + 0)) &&
        ((*s1 + 1) == *(s1 + 1) & *(s2 + 1)) &&
        ((*s1 + 2) == *(s1 + 2) & *(s2 + 2)) &&
        ((*s1 + 3) == *(s1 + 3) & *(s2 + 3)) &&
        ((*s1 + 4) == *(s1 + 4) & *(s2 + 4));
#endif
}

void bt_str2sha1hash(char *hash_out, const char *str, int len)
{
    SHA1_CTX ctx;
    int ii;

    SHA1Init(&ctx);
    for (ii=0; ii<len; ii+=1)
        SHA1Update(&ctx, str + ii, 1);
    SHA1Final(hash_out, &ctx);
    hash_out[20] = '\0';
}
