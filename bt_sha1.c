
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include <arpa/inet.h>

#include "sha1.h"

#include "block.h"
#include "bt.h"
#include "bt_local.h"


int bt_sha1_equal(
    uint32_t * s1,
    uint32_t * s2
)
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


char *str2sha1hash(
    const char *str,
    int len
)
{
#if 0
    SHA1Context sha;

    SHA1Reset(&sha);
    SHA1Input(&sha, str, len);
    if (!SHA1Result(&sha))
    {
        fprintf(stderr, "sha: could not compute message digest for %s\n", str);
        return NULL;
    }
    else
    {
        char *hash;

        int ii;

#if 1
        for (ii = 0; ii < 5; ii++)
            sha.Message_Digest[ii] = htonl(sha.Message_Digest[ii]);
        hash = malloc(21);
        memcpy(hash, sha.Message_Digest, 20);
        hash[20] = '\0';
#else
        asprintf(&hash,
                 "%08x%08x%08x%08x%08x",
                 sha.Message_Digest[0],
                 sha.Message_Digest[1],
                 sha.Message_Digest[2],
                 sha.Message_Digest[3], sha.Message_Digest[4]);
#endif
//        printf("hash: %s\n", hash);
        return hash;
    }
#else

#if 1    // REID
    SHA1_CTX ctx;

    int ii;

    unsigned char hash[20];

    SHA1Init(&ctx);
    for (ii=0; ii<len; ii+=1)
        SHA1Update(&ctx, str + ii, 1);
    SHA1Final(hash, &ctx);

    char *out;

    out = malloc(21);
    memcpy(out, hash, 20);
    out[20] = '\0';
    return out;
#else // AARON GRIFFORD
    SHA_CTX ctx;

    unsigned char hash[20];

    SHA1_Init(&ctx);
    SHA1_Update(&ctx, str, len);
    SHA1_Final(hash, &ctx);

    char *out;

    out = malloc(21);
    memcpy(out, hash, 20);
    out[20] = '\0';
    return out;
#endif
#endif
}
