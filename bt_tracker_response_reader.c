#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bencode.h"

static void __do_peer_list(
    void *id,
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

        long int current_file_len = 0;

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
//        assert(peerid_len == 20);
            bt_client_add_peer(id, peerid, peerid_len, ip, ip_len, port);
        }
        else
        {
            assert(0);
        }

    }
}

int bt_client_read_tracker_response(
    void *id,
    unsigned char *buf,
    int len
)
{
    bencode_t ben;

    bencode_init(&ben, buf, len);

//    printf("bencode->len = %d.\n%.*s\n", len, len, buf);

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
            printf("ERROR\n");
            return 0;
        }

//        printf("key: %.*s\n", klen, key);

/* This key is OPTIONAL. If present, the dictionary MUST NOT contain any other keys. The peer should interpret this as if the attempt to join the torrent failed. The value is a human readable string containing an error message with the failure reason. */
        if (!strncmp(key, "failure reason", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            bt_client_set_failed(id, val, len);
            return 1;
        }
/* A peer must send regular HTTP GET requests to the tracker to obtain an updated list of peers and update the tracker of its status. The value of this key indicated the amount of time that a peer should wait between to consecutive regular requests. This key is REQUIRED. */
        else if (!strncmp(key, "interval", klen))
        {
            long int interval;

            bencode_int_value(&benk, &interval);
            bt_client_set_interval(id, interval);
        }
/* This is an integer that indicates the number of seeders. This key is OPTIONAL. */
        else if (!strncmp(key, "complete", klen))
        {
            long int ncomplete_peers;

            bencode_int_value(&benk, &ncomplete_peers);
            bt_client_set_num_complete_peers(id, ncomplete_peers);
        }
/* This is an integer that indicates the number of peers downloading the torrent. This key is OPTIONAL. */
        else if (!strncmp(key, "incomplete", klen))
        {
            long int nincomplete_peers;

            bencode_int_value(&benk, &nincomplete_peers);
            bt_client_set_num_complete_peers(id, nincomplete_peers);
        }
/* This is a bencoded list of dictionaries containing a list of peers that must be contacted in order to download a file. This key is REQUIRED. It has the following structure: */
        else if (!strncmp(key, "peers", klen))
        {
            /*  compact=0 */
            if (bencode_is_list(&benk))
            {
                __do_peer_list(id, &benk);
            }
            /*  compact=1 */
            /* use the bittorrent compact peer format listing */
            else if (bencode_is_string(&benk))
            {
                int len, ii;

                const unsigned char *val;

                bencode_string_value(&benk, (const char **) &val, &len);
                for (ii = 0; ii < len; ii += 6, val += 6)
                {
                    unsigned char ip[32], *str;

//              printf("adding ip: %d.%d.%d.%d:%d", val[0], val[1], val[2],
//                           val[3], ((int) val[4] << 8) | (int) val[5]);
                    sprintf(ip, "%d.%d.%d.%d", val[0], val[1], val[2], val[3]);
                    bt_client_add_peer(id, NULL, 0, ip, strlen(ip),
                                       ((int) val[4] << 8) | (int) val[5]);
                }
            }
        }
    }

    return 1;
}
