
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <arpa/inet.h>

#include "bt.h"

#define PROTOCOL_NAME "BitTorrent protocol"
#define INFOKEY_LEN 20
#define BLOCK_SIZE 1 << 14      // 16kb
#define PWP_HANDSHAKE_RESERVERD "\0\0\0\0\0\0\0\0"
#define VERSION_NUM 1000
#define PEER_ID_LEN 20

typedef unsigned char byte;

typedef struct
{
    /* index on 'bit stream' */
    int idx;

//    int data_size;
    byte *data;
    /* progress of piece
     * if there is one vblock that is stretched over the whole piece, then we
     * are done*/
//    int nvblocks;
    /* size of array */
//    var_block_t *vblocks;
} bt_piece_t;

typedef struct
{
    int piece_idx;
    int block_byte_offset;
    int block_len;
} pwp_piece_block_t;

typedef struct
{
    int length;

    /* 32 characters representing the md5sum of the file */
    char *md5sum;
    char *path;
} bt_file_t;

typedef struct
{
    int idx;

//    int nblocks;
    /* a list of files */
//    bt_file_t *files;
//    int nfiles;
    /* a string containing the 20 byte sha1 of every file, concatenated.
     * sha1 hash = 20 bytes */
    char *pieces_hash;
    int piece_len;
    int npieces;

    /* progress */
    bt_piece_t *pieces;
    char *infohash;

    /* 20-byte self-designated ID of the peer */
    char *peer_id;

    /* name of the topmost directory */
    char *name;
    char *tracker_url;
    int trackerclient_id;

    /* listen for pwp messages on this port */
    int pwp_port;

#if 0
    void (
    *func_listen
    )    (
    int id
    );
#endif
//    bt_peer_state_t *peerstates;
//    bt_peer_t *peers;
    int npeers;

    /* sha1 hash of the info_hash */
    char *info_hash;

    /* the main directory of the file */
    char *path;

    /* net stuff */

    bt_net_funcs_t net;

    void *net_udata;

    /* how often we must send messages to the tracker */
    int interval;

    /* number of complete peers */
    int ncomplete_peers;

    char fail_reason[255];

    /* net tracker id */
    int nt_id;
} bt_client_t;

/*----------------------------------------------------------------------------*/
typedef struct
{


} trackerclient_t;

static bt_client_t *__clients[10];

static bt_client_t *__get_bt(
    int id
)
{
    return __clients[id];
}

/**
 * Initiliase the bittorrent client
 * */
int bt_init(
)
{
    bt_client_t *bt;

    bt = calloc(1, sizeof(bt_client_t));
    bt->pwp_port = 6000;
    bt->idx = 0;
    __clients[0] = bt;
    return 0;
}

int bt_read_metainfo_file(
    const int id,
    const char *fname
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    char *contents;

    int len;

    contents = readFile(fname, &len);
    bt_read_metainfo(id, contents, len);
    free(contents);
}

#if 0
void bt_set_info_key(
    int id,
    const char *info_key
)
{
#if 0
    tc->info_key = info_key;
#endif
}
#endif

void bt_add_tracker_backup(
    int id,
    char *url,
    int url_len
)
{
    printf("backup tracker url: %.*s\n", url_len, url);
}

#if 0
int bt_add_peer(
    int id,
    bt_peer_t * new_peer
)
{
    int ii;

    int unused_slot;

    bt_client_t *bt;

    bt = __get_bt(id);
    /* can't have same peer_id as me! DROP */
    if (!strncmp(bt->peer_id, new_peer->peer_id))
    {
        return -1;
    }

    unused_slot = -1;
    for (ii = 0; ii < tc->npeers; ii++)
    {
        bt_peer_t *peer;

        peer = &tc->peers[ii];
        /* consider that this is an unused slot */
        if (peer->peer_id == NULL)
        {
            /* remember this slot, so we can recycle it */
            if (-1 != unused_slot)
            {
                unused_slot = ii;
            }
            continue;
        }

        /* can't have same peer_id as someone I already have! DROP */
        if (!strncmp(peer->peer_id, new_peer->peer_id))
        {
            return -1;
        }
    }

    if (ii >= tc->npeers)
    {
        __append_peer_list(tc);
    }

    return -1;
}
#endif

void bt_connect_to_peer(
    int id,
    int peer_id
)
{
// 1. Any remote peer wishing to communicate with the local peer must open a TCP connection to this port and perform a handshake operation

}

static void __bt_receive(
    bt_client_t * bt
)
{
    // 1. receive from tcp socket
    // 2. figure out what the data is

#if 0
    if (bt->func_listen(bt->idx))
    {

    }
#endif


}


/*----------------------------------------------------------------------------*/

#if 0
void bt_pwp_listen_on_port(
    int id,
    int port
)
{

}
#endif

/*----------------------------------------------------------------------------*/

/**
 * peer connections are given this as a callback whenever they want to send
 * information */
static int __peerconn_send_to_peer(
    bt_t * bt,
    bt_peer_t * peer,
    void *data,
    int len
)
{


}

/**
 * add the peer; initiate the connection
 */
int bt_add_peer(
    int id,
    const char *peer_id,
    int peer_id_len,
    const char *ip,
    int ip_len,
    int port
)
{
    void *pc;

    bt_client_t *bt;

    bt = __get_bt(id);

    pc = bt_peerconn_new();

    bt_peerconn_set_send_udata(pc, bt);
    bt_peerconn_set_send_func(pc, __peerconn_send_to_peer);
    bt_peerconn_set_getpiece_func(pc, __peerconn_send_to_peer);
    /* the remote peer will have always send a handshake */
    bt_peerconn_send_handshake(pc, bt->infohash, bt->peer_id);

    return 0;
}

int bt_remove_peer(
    int id,
    const char *peer_id
)
{

    return 0;
}

int bt_is_done(
    int id
)
{
    return 1;
}


/*
'announce':
    This is a string value. It contains the announce URL of the tracker. 
'announce-list':
    This is an OPTIONAL list of string values. Each value is a URL pointing to a backup tracker. This value is not used in BTP/1.0. 
'comment':
    This is an OPTIONAL string value that may contain any comment by the author of the torrent. 
'created by':
    This is an optional string value and may contain the name and version of the program used to create the metainfo file. 
'creation date':
    This is an OPTIONAL string value. It contains the creation time of the torrent in standard Unix epoch format. 
'info':
    This key points to a dictionary that contains information about the files to download. The entries are explained in the following sections. 

4.1.1 Single File Torrents

If the torrent only specifies one file, the info dictionary must have the following keys:

'length':
    This is an integer value indicating the length of the file in bytes. 
'md5sum':
    This is an OPTIONAL value. If included it must be a string of 32 characters corresponding to the MD5 sum of the file. This value is not used in BTP/1.0. 
'name':
    A string containing the name of the file. 
'piece length':
    An integer indicating the number of bytes in each piece. 
'pieces':
    This is a string value containing the concatenation of the 20-byte SHA1 hash value for all pieces in the torrent. For example, the first 20 bytes of the string represent the SHA1 value used to verify piece index 0. 

4.1.2 Multi File Torrents

If the torrent specifies multiple files, the info dictionary must have the following structure:

'files':
    This is a list of dictionaries. Each file in the torrent has a dictionary associated to it having the following structure:

        'length':
            This is an integer indicating the total length of the file in bytes. 
        'md5sum':
            This is an OPTIONAL value. if included it must be a string of 32 characters corresponding to the MD5 sum of the file. This value is not used in BTP/1.0. 
        'path':
            This is a list of string elements that specify the path of the file, relative to the topmost directory. The last element in the list is the name of the file, and the elements preceding it indicate the directory hierarchy in which this file is situated. 

'name':
    This is a string value. It contains the name of the top-most directory in the file structure. 
'piece length':
    This is an integer value. It contains the number of bytes in each piece. 
'pieces':
    This is a string value. It must contain the concatenation of all 20-byte SHA1 hash values that are used by BTP/1.0 to verify each downloaded piece. The first 20 bytes of the string represent the SHA1 value used to verify piece index 0. 

        */

/**
 * @return -1 on error */
int bt_add_piece(
    int id,
    const char *piece
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
//    printf("got a piece '%.*s'\n", 20, piece);
}

bt_piece_t *bt_get_piece(
    bt_client_t * bt,
    const int piece_idx
)
{
    return NULL;
}

void bt_add_pieces(
    int id,
    const char *pieces,
    int len
)
{
    int prog;

    prog = 0;
    while (prog <= len)
    {
        prog++;
        if (0 == prog % 20)
        {
            //printf("%d\n", prog);
            bt_add_piece(id, pieces);
            pieces += 20;
        }
    }
}

#if 1
int bt_add_file(
    int id,
    char *fname,
    int fname_len,
    long int flen
)
{

    return 0;
}
#endif

char *url2host(
    const char *url
)
{
    const char *host;

    assert(!strncmp(url, "http://", 7));
    host = url + strlen("http://");
    return strndup(host, strpbrk(host, ":/") - host);
}

static void __build_request(
    bt_client_t * bt,
    char **request
)
{
    asprintf(request,
             "GET %s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%d&numwant=200 http/1.0\r\n"
             "\r\n\r\n",
             bt->tracker_url,
             bt->info_hash,
             bt->peer_id, bt->pwp_port, 0, 0, bt->npieces * bt->piece_len);
}

/**
 * 
 * Send request to tracker.
 * Get peer listing.
 * */
void bt_connect_to_tracker(
    int id
//    int tracker_id
)
{
    bt_client_t *bt;

    char *response, *request = NULL;

    bt = __get_bt(id);
    assert(bt);
    assert(bt->tracker_url);
    assert(bt->info_hash);
    assert(bt->peer_id);
//    asprintf(&request, "GET %s/?info_hash=%s&peer_id=%s", bt->tracker_url,
//             bt->info_hash, bt->peer_id);
    __build_request(bt, &request);
    char *host;

    int rlen;

    host = url2host(bt->tracker_url);
    printf("sending on: '%s'\n", host);
//    response = http_get_response_from_request(host, "9000", request);
    bt->net.tracker_connect(&bt->net_udata, host, "9000", &bt->nt_id);
    printf("request: %s\n", request);
    bt->net.tracker_send(&bt->net_udata, bt->nt_id, request, strlen(request));
    printf("sending\n");
    bt->net.tracker_recv(&bt->net_udata, bt->nt_id, &response, &rlen);
    const char *document;

    document = strstr(response, "\r\n\r\n");
    bt_read_tracker_response(id, document, strlen(document));
    printf("response: %s\n", response);
    free(host);
    free(request);
    free(response);
}

/*----------------------------------------------------------------------------*/

void bt_set_num_complete_peers(
    int id,
    int npeers
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    bt->ncomplete_peers = npeers;
}

int bt_is_failed(
    int id
)
{

}

void bt_set_failed(
    int id,
    const char *reason
)
{

}

void bt_set_interval(
    int id,
    int interval
)
{

}

void bt_set_info_hash(
    int id,
    char *info_hash
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    bt->info_hash = info_hash;
//    printf("setting info hash: %s\n", info_hash);
}

char *bt_get_info_hash(
    int id
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    return bt->info_hash;
}

char *bt_get_fail_reason(
    int id
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    return bt->fail_reason;
}

int bt_get_interval(
    int id
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    return bt->interval;
}

void bt_set_peer_id(
    int id,
    char *peer_id
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    bt->peer_id = peer_id;
    printf("peer_id = %s\n", bt->peer_id);
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

void bt_step(
    int id
)
{
    // get peer list from tracker
    // select most rarest piece
    // tell peer that we are interested
//    __pwp_send_interested();
    // find peer with rarest piece
    // request piece from peer
    // __pwp_request_piece_from_peer(peer,piece);


//    pwp_step(bt);
}

int bt_get_nbytes_downloaded(
    int id
)
{
    //FIXME_STUB;
    return 0;
}

int bt_release(
    int id
)
{
    //FIXME_STUB;
}

/*
 *
 The size of a piece is determined by the publisher of the torrent. A good recommendation is to use a piece size so that the metainfo file does not exceed 70 kilobytes.

 */

// number_of_blocks = (fixed_piece_size / fixed_block_size)
//                       + !!(fixed_piece_size % fixed_block_size)

/**
 * How many pieces are there of this file?
 * Note: bt_read_torrent_file sets this */
void bt_set_npieces(
    int id,
    int npieces
)
{
// fixed_piece_size = size_of_torrent / number_of_pieces

    bt_client_t *bt;

    bt = __get_bt(id);
    bt->npieces = npieces;
}

void bt_set_tracker_url(
    const int id,
    const char *url,
    const int len
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    bt->tracker_url = malloc(sizeof(char) * (len + 1));
    strncpy(bt->tracker_url, url, len);
}

char *bt_get_tracker_url(
    const int id
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    return bt->tracker_url;
}

void bt_set_path(
    int id,
    const char *path
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    bt->path = strdup(path);
}

void bt_set_piece_length(
    int id,
    int len
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    bt->piece_len = len;
}

int bt_get_num_peers(
    int id
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    return 0;
}

void bt_set_net_funcs(
    int id,
    bt_net_funcs_t * net
)
{
    bt_client_t *bt;

    bt = __get_bt(id);
    memcpy(&bt->net, net, sizeof(bt_net_funcs_t));
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
