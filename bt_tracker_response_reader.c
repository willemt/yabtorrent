#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bencode.h"

static void __do_peer_dict(
    int id,
    bencode_t * dict
)
{
    const char *peerid, *ip;

    int port;

    int peerid_len, ip_len;

    port = 0;

    while (bencode_dict_has_next(dict))
    {
        const char *key;

        int klen;

        bencode_t benk;

        bencode_dict_get_next(dict, &benk, &key, &klen);

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
        bt_add_peer(id, peerid, peerid_len, ip, ip_len, port);
    }
    else
    {
        assert(0);
    }
}

int bt_read_tracker_response(
    int id,
    char *buf,
    int len
//    const char *fname
)
{
    bencode_t ben;

    bencode_init(&ben, buf, len);
//    printf("bencode->len = %d\n", len);

    if (!bencode_is_dict(&ben))
        return;

    while (bencode_dict_has_next(&ben))
    {
        int klen;

        const char *key;

        bencode_t benk;

        bencode_dict_get_next(&ben, &benk, &key, &klen);
//        printf("zey: %.*s\n", klen, key);
        // This key is OPTIONAL. If present, the dictionary MUST NOT contain any other keys. The peer should interpret this as if the attempt to join the torrent failed. The value is a human readable string containing an error message with the failure reason.
        if (!strncmp(key, "failure reason", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            bt_set_failed(id, val, len);
            return 1;
        }
// A peer must send regular HTTP GET requests to the tracker to obtain an updated list of peers and update the tracker of its status. The value of this key indicated the amount of time that a peer should wait between to consecutive regular requests. This key is REQUIRED. 
        else if (!strncmp(key, "interval", klen))
        {
            long int interval;

            bencode_int_value(&benk, &interval);
            bt_set_interval(id, interval);
        }
// This is an integer that indicates the number of seeders. This key is OPTIONAL. 
        else if (!strncmp(key, "complete", klen))
        {
            long int ncomplete_peers;

            bencode_int_value(&benk, &ncomplete_peers);
            bt_set_num_complete_peers(id, ncomplete_peers);
        }
// This is an integer that indicates the number of peers downloading the torrent. This key is OPTIONAL. 
        else if (!strncmp(key, "incomplete", klen))
        {
            long int nincomplete_peers;

            bencode_int_value(&benk, &nincomplete_peers);
            bt_set_num_complete_peers(id, nincomplete_peers);
        }
// This is a bencoded list of dictionaries containing a list of peers that must be contacted in order to download a file. This key is REQUIRED. It has the following structure:
        else if (!strncmp(key, "peers", klen))
        {
            __do_peer_dict(id, &benk);
        }
    }

    return 1;
}
