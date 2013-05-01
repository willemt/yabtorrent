
/**
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    SHA1Update(&ctx, str, len);
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
