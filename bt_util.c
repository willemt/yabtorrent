
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
 *
 * @section description
 * for now this file will be used to hold misc operations
 */



#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <arpa/inet.h>

#include "bt.h"
#include "bt_local.h"

#include "sha1.h"

#define HTTP_PREFIX "http://"

#if 0
/**
 * Convert URL into HOSTNAME and PORT components
 * */
void url2comps(
    const char *url,
    char **host,
    int *host_len char **port,
    int *port_len
)
{
    *host = url;

//    printf("url: %s\n", url);
//    assert(!strncmp(url, "http://", 7));

    if (!strncmp(url, HTTP_PREFIX, strlen(HTTP_PREFIX)))
    {
        *host += strlen("http://");
    }

    *host_len = strpbrk(host, ":/") - host;

    return strndup(host,);
}
#endif

/**
 * Obtain hostname from URL */
char *url2host(
    const char *url
)
{
    const char *host;

    host = url;

//    printf("url: %s\n", url);
//    assert(!strncmp(url, "http://", 7));

    if (!strncmp(url, HTTP_PREFIX, strlen(HTTP_PREFIX)))
    {
        host += strlen("http://");
    }

    return strndup(host, strpbrk(host, ":/") - host);
}


char *url2port(
    const char *url
)
{
    const char *port, *host;

//    assert(!strncmp(url, "http://", 7));
    host = url;
    if (!strncmp(url, HTTP_PREFIX, strlen(HTTP_PREFIX)))
    {
        host += strlen("http://");
    }

    port = strpbrk(host, ":/") + 1;
    return strndup(port, strpbrk(port, "/") - port);
}

char *bt_generate_peer_id(
)
{
    char *str;

    int rand_num;

    rand_num = random();
    asprintf(&str, "-AA-%d-%011d", VERSION_NUM, rand_num);
    assert(strlen(str) == PEER_ID_LEN);
//    printf("random - %d\n", rand_num);
//    printf("random: %s\n", str);
//    printf("len: %d\n", strlen(str));
    return str;
}

#if 0
void bt_sha1_string(
    char *str,
    const int len,
    char *out
)
{
#if 0
    SHA1_CTX context;

//    uint8_t digest[20];
//    char output[80];
    SHA1_Init(&context);
    SHA1_Update(&context, (uint8_t *) str, len);
    SHA1_Final(&context, out);
//      digest_to_hex(digest, output);
#endif
    SHA1Context sha;

    SHA1Reset(&sha);
    SHA1Input(&sha, str, len);
    SHA1Result(&sha);
    int ii;

    for (ii = 0; ii < 5; ii++)
        sha.Message_Digest[ii] = htonl(sha.Message_Digest[ii]);
    memcpy(out, sha.Message_Digest, sizeof(char) * 20);
#if 0
    if (strcmp(output, test_results[k]))
    {
        fprintf(stdout, "FAIL\n");
        fprintf(stderr, "* hash of \"%s\" incorrect:\n", test_data[k]);
        fprintf(stderr, "\t%s returned\n", output);
        fprintf(stderr, "\t%s is correct\n", test_results[k]);
        return (1);
    }
#endif

#if 0
    /* million 'a' vector we feed separately */
    SHA1_Init(&context);
    for (k = 0; k < 1000000; k++)
        SHA1_Update(&context, (uint8_t *) "a", 1);
    SHA1_Final(&context, digest);
    digest_to_hex(digest, output);
    if (strcmp(output, test_results[2]))
    {
        fprintf(stdout, "FAIL\n");
        fprintf(stderr, "* hash of \"%s\" incorrect:\n", test_data[2]);
        fprintf(stderr, "\t%s returned\n", output);
        fprintf(stderr, "\t%s is correct\n", test_results[2]);
        return (1);
    }

    /* success */
    fprintf(stdout, "ok\n");
    return (0);
#endif
}
#endif

#if 1
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
#endif

/*----------------------------------------------------------------------------*/
