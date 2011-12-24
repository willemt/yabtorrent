/*
 * =====================================================================================
 *
 *       Filename:  bt_sha1.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/24/11 18:33:24
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#include "sha1.h"

#include "bt.h"
#include "bt_local.h"

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
