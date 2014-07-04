
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 

 * @file
 * @brief Manage connection with tracker
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "bencode.h"
#include "tracker_client.h"
#include "tracker_client_private.h"

static void __do_peer_list(
    trackerclient_t* me,
    bencode_t * list
)
{
    const char *peerid, *ip;
    long int port;
    int peerid_len, ip_len;

    port = 0;

    while (bencode_list_has_next(list))
    {
        bencode_t dict;

        bencode_list_get_next(list, &dict);

        while (bencode_dict_has_next(&dict))
        {
            const char *key;
            int klen;
            bencode_t benk;

            bencode_dict_get_next(&dict, &benk, &key, &klen);

            if (!strncmp(key, "peer id", klen))
            {
                bencode_string_value(&benk, &peerid, &peerid_len);
            }
            else if (!strncmp(key, "ip", klen))
            {
                bencode_string_value(&benk, &ip, &ip_len);
            }
            else if (!strncmp(key, "port", klen))
            {
                bencode_int_value(&benk, &port);
            }
        }

        if (peerid && ip && port != 0)
        {
            assert(peerid_len == 20);
//            me->funcs.add_peer(me->caller, peerid, peerid_len, ip, ip_len, port);
        }
        else
        {
            assert(0);
        }
    }
}

int trackerclient_read_tracker_response(
    trackerclient_t* me,
    const char *buf,
    int len
)
{
    bencode_t ben;

    bencode_init(&ben, buf, len);

    if (!bencode_is_dict(&ben))
    {
        return 0;
    }

    while (bencode_dict_has_next(&ben))
    {
        int klen;
        const char *key;
        bencode_t benk;

        if (0 == bencode_dict_get_next(&ben, &benk, &key, &klen))
        {
            // ERROR dict is invalid
            return 0;
        }

        /* This key is OPTIONAL. If present, the dictionary MUST NOT contain
         * any other keys. The peer should interpret this as if the attempt to
         * join the torrent failed. The value is a human readable string
         * containing an error message with the failure reason. */
        if (!strncmp(key, "failure reason", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            return 1;
        }
        /* A peer must send regular HTTP GET requests to the tracker to obtain
         * an updated list of peers and update the tracker of its status. The
         * value of this key indicated the amount of time that a peer should
         * wait between to consecutive regular requests. This key is REQUIRED*/
        else if (!strncmp(key, "interval", klen))
        {
            long int interval;

            bencode_int_value(&benk, &interval);
        }
        /* This is an integer that indicates the number of seeders. This key is
         * OPTIONAL. */
        else if (!strncmp(key, "complete", klen))
        {
            long int ncomplete_peers;

            bencode_int_value(&benk, &ncomplete_peers);
        }
        /* This is an integer that indicates the number of peers downloading
         * the torrent. This key is OPTIONAL. */
        else if (!strncmp(key, "incomplete", klen))
        {
            long int nincomplete_peers;

            bencode_int_value(&benk, &nincomplete_peers);
        }
        /*
         * This is a bencoded list of dictionaries containing a list of peers 
         * that must be contacted in order to download a file. This key is 
         * REQUIRED. It has the following structure: */
        else if (!strncmp(key, "peers", klen))
        {
            /*  compact=0 */
            if (bencode_is_list(&benk))
            {
                __do_peer_list(me, &benk);
            }
            /*  compact=1 */
            /* use the bittorrent compact peer format listing */
            else if (bencode_is_string(&benk))
            {
                int len, ii;

                const unsigned char *val;

                if (0 == bencode_string_value(&benk, (const char **) &val, &len))
                {
                    // ERROR: string is invalid
                    return 0;
                }

                for (ii = 0; ii < len; ii += 6, val += 6)
                {
                    char ip[32];

                    sprintf(ip, "%d.%d.%d.%d", val[0], val[1], val[2], val[3]);

                    if (!strcmp(ip,"0.0.0.0")) continue;
                    me->on_add_peer(me->callee, NULL, 0, ip, strlen(ip),
                                       ((int) val[4] << 8) | (int) val[5]);
                }
            }
        }
    }

    return 1;
}
