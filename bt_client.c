#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <string.h>
#include <stdarg.h>

#include <arpa/inet.h>

#include "bt.h"
#include "bt_local.h"
#include "url_encoder.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>


/*
 * bt_client
 *  has a bt_peerconn_t for each peer_t
 *
 * peerconnection_t has a peer_t
 *  downloads from peer
 *  manages connection with peer
 *  adds blocks to piece_t
 *
 * piecedb_t
 *  stores pieces
 *  decides which piece is best
 *
 * piece_t
 *  manage progress of self
 *
 * filedumper
 *  maps pieces to files
 *  dumps files to disk
 *  reads files from disk
 *
 *
 */

typedef struct
{
    bt_piece_info_t pinfo;
    bt_piecedb_t *db;

    /*  disk cache */
    void *dc;

    /*  file dumper */
    bt_filedumper_t *fd;

    /* 20-byte self-designated ID of the peer */
    char *p_peer_id;

    /* name of the topmost directory */
    char *name;

    /* the main directory of the file */
    char *path;

    char *tracker_url;
    int trackerclient_id;
    void **peerconnects;
    int npeers;

    /* sha1 hash of the info_hash */
    char *info_hash;

    /* net stuff */

    bt_net_funcs_t net;

    void *net_udata;

    /* how often we must send messages to the tracker */
    int interval;

    /* number of complete peers */
    int ncomplete_peers;

    char fail_reason[255];

    /* so that we remember when we last requested the peer list */
    time_t last_tracker_request;

    /*  are we seeding? */
    int am_seeding;

    /*------------------------------------------------*/

    /*  don't seed, shutdown when complete */
    int o_shutdown_when_complete;
    /*  how many seconds between tracker scrapes */
//    int o_tracker_scrape_interval;

    /*------------------------------------------------*/

    /*  logging  */

//    bt_logger_t logger;
    func_log_f func_log;
    void *log_udata;

    /*  this holds my IP. I figure it out */
    char my_ip[32];
//    char my_port[8];

    /* listen for pwp messages on this port */
    int pwp_listen_port;

    bt_client_cfg_t cfg;
    bt_pwp_cfg_t pwpcfg;

    /*  leeching choker */
    void *lchoke;

    void *ticker;
} bt_client_t;

/*----------------------------------------------------------------------------*/

/**
 * print a string of all the downloaded pieces */
static void __print_pieces_downloaded(
    void *bto
)
{
    bt_client_t *bt = bto;

    int ii, counter, is_complete = 1;

    printf("pieces downloaded: ");

    for (ii = 0, counter = 20;
         ii < bt_piecedb_get_length(bt->db); ii++, counter += 1)
    {
        bt_piece_t *pce;

        pce = bt_piecedb_get(bt->db, ii);

        if (bt_piece_is_complete(pce))
        {
            printf("1");
        }
        else
        {
            printf("0");
            is_complete = 0;
        }

        if (counter == 80)
        {
            counter = 2;
            printf("\n  ");
        }
    }
    printf("\n");

    if (is_complete)
    {

    }
}

/**
 * @return 1 if all complete, 0 otherwise */
static int __all_pieces_are_complete(
    bt_client_t * bt
)
{
    int ii;

    for (ii = 0; ii < bt_piecedb_get_length(bt->db); ii++)
    {
        bt_piece_t *pce = bt_piecedb_get(bt->db, ii);

        if (!bt_piece_is_complete(pce))
        {
            return 0;
        }
    }

    return 1;
}

static void __log(
    void *bto,
    void *src,
    const char *fmt,
    ...
)
{
    bt_client_t *bt = bto;
    char buf[1024];

    va_list args;

    if (!bt->func_log)
        return;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    bt->func_log(bt->log_udata, NULL, buf);
}

static void __FUNC_peerconn_log(
    void *bto,
    void *src_peer,
    const char *buf,
    ...
)
{
    bt_client_t *bt = bto;

    bt_peer_t *peer = src_peer;

    char buffer[256];

    sprintf(buffer, "pwp,%s,%s\n", peer->peer_id, buf);
    bt->func_log(bt->log_udata, NULL, buffer);
}

static int __FUNC_peerconn_pieceiscomplete(
    void *bto,
    void *piece
)
{
    bt_client_t *bt = bto;

    bt_piece_t *pce = piece;

    return bt_piece_is_complete(pce);
}

/**
 *
 * @return info hash */
static char *__FUNC_peerconn_get_infohash(
    void *bto,
    bt_peer_t * peer
)
{
    bt_client_t *bt = bto;

    return bt->info_hash;
}

static int __FUNC_peerconn_disconnect(
    void *udata,
    bt_peer_t * peer,
    char *reason
)
{

    printf("DISCONNECTING: %s\n", reason);
    return 1;
}

static int __FUNC_peerconn_connect(
    void *bto,
    void *pc,
    bt_peer_t * peer
)
{
    bt_client_t *bt = bto;

    /* the remote peer will have always send a handshake */
    if (!bt->net.peer_connect)
    {
        return 0;
    }

//    printf("connecting to peer (%s:%s)\n", peer->ip, peer->port);
    if (0 == bt->net.peer_connect(&bt->net_udata, peer->ip,
                                  peer->port, &peer->net_peerid))
    {
        printf("failed\n");
        return 0;
    }

#if 0
    printf("handshaking with peer: %d...\n", peer->net_peerid);
    bt_peerconn_set_active(pc, TRUE);

    if (0 == bt_peerconn_send_handshake(pc, bt->info_hash, bt->p_peer_id))
    {
        bt_peerconn_set_active(pc, FALSE);
        return 0;
    }
#endif

    return 1;
}

/*----------------------------------------------------------------------------*/
int __get_drate(
    const void *bto,
    const void *pc
)
{
    const bt_client_t *bt = bto;

    return bt_peerconn_get_download_rate(pc);
}

int __get_urate(
    const void *bto,
    const void *pc
)
{
    const bt_client_t *bt = bto;

    return bt_peerconn_get_upload_rate(pc);
}

int __get_is_interested(
    void *bto,
    void *pc
)
{
    bt_client_t *bt = bto;

    return bt_peerconn_is_interested(pc);
}

void __choke_peer(
    void *bto,
    void *pc
)
{

    bt_client_t *bt = bto;

    bt_peerconn_choke(pc);
//    pc->isChoked = 1;
}

void __unchoke_peer(
    void *bto,
    void *pc
)
{
    bt_client_t *bt = bto;

//    pr->isChoked = 0;
    bt_peerconn_unchoke(pc);
//    pc->isChoked = 1;
}

bt_choker_peer_i iface_choker_peer = {
    .get_drate = __get_drate,.get_urate =
        __get_urate,.get_is_interested =
        __get_is_interested,.choke_peer =
        __choke_peer,.unchoke_peer = __unchoke_peer
};

static void __leecher_peer_reciprocation(
    void *bto
)
{
    bt_client_t *bt = bto;

    bt_leeching_choker_decide_best_npeers(bt->lchoke);
    bt_ticker_push_event(bt->ticker, 10, bt, __leecher_peer_reciprocation);
}

static void __leecher_peer_optimistic_unchoke(
    void *bto
)
{
    bt_client_t *bt = bto;

    bt_leeching_choker_optimistically_unchoke(bt->lchoke);
    bt_ticker_push_event(bt->ticker, 30, bt, __leecher_peer_optimistic_unchoke);
}

/*----------------------------------------------------------------------------*/
/**
 * Initiliase the bittorrent client
 */
void *bt_client_new(
)
{
    bt_client_t *bt;

    bt = calloc(1, sizeof(bt_client_t));

    bt->ticker = bt_ticker_new();
    bt->db = bt_piecedb_new();
    bt->fd = bt_filedumper_new();
    bt->dc = bt_diskcache_new();

    /* configuration */
    sprintf(bt->my_ip, "127.0.0.1");
    bt->pwp_listen_port = 6000;
    bt->cfg.max_peer_connections = 10;
    bt->cfg.select_timeout_msec = 1000;
    bt->cfg.tracker_scrape_interval = 10;
    bt->cfg.max_active_peers = 4;
    bt->pwpcfg.max_pending_requests = 10;

    /*  set leeching choker */
    bt->lchoke = bt_leeching_choker_new(bt->cfg.max_active_peers);
    bt_leeching_choker_set_choker_peer_iface(bt->lchoke, bt,
                                             &iface_choker_peer);

    /*  set diskcache logger and blockrw */
    bt_diskcache_set_disk_blockrw(bt->dc,
                                  bt_filedumper_get_blockrw(bt->fd), bt->fd);
    bt_diskcache_set_func_log(bt->dc, __log, bt);

    /* point piece database to diskcache */
    bt_piecedb_set_diskstorage(bt->db,
                               bt_diskcache_get_blockrw(bt->dc), bt->dc);

    /*  start reciprocation timer */
    bt_ticker_push_event(bt->ticker, 10, bt, __leecher_peer_reciprocation);

    /*  start optimistic unchoker timer */
    bt_ticker_push_event(bt->ticker, 30, bt, __leecher_peer_optimistic_unchoke);
//    bt_piecedb_set_filelist(bt->db, bt->fd);
//    sprintf(bt->my_port, "9000");
//    bt_piecedb_set_piece_info(bt->db, &bt->pinfo);
    return bt;
}

int bt_client_read_metainfo_file(
    void *bto,
    const char *fname
)
{
    bt_client_t *bt = bto;
    char *contents;
    int len;

    if (!(contents = read_file(fname, &len)))
    {
        printf("ERROR: torrent file: %s can't be read\n", fname);
        return 0;
    }

    bt_client_read_metainfo(bto, contents, len, &bt->pinfo);
    free(contents);
    printf("numpieces: %d\n", bt_client_get_num_pieces(bto));
    __print_pieces_downloaded(bto);
    if (__all_pieces_are_complete(bt))
    {
        bt->am_seeding = 1;
    }

    return 1;
}

static void __build_tracker_request(
    bt_client_t * bt,
    char **request
)
{
    bt_piece_info_t *bi;
    char *info_hash_encoded;

    bi = &bt->pinfo;
    info_hash_encoded = url_encode(bt->info_hash);
    asprintf(request,
//             "GET %s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%d&numwant=200 http/1.0\r\n"
             "GET %s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%d&event=started&compact=1 http/1.0\r\n"
             "\r\n\r\n",
             bt->tracker_url,
             info_hash_encoded,
             bt->p_peer_id, bt->pwp_listen_port, 0, 0,
             bi->npieces * bi->piece_len);
    free(info_hash_encoded);
}

static int __get_tracker_request(
    void *bto
)
{
    bt_client_t *bt = bto;
    int status = 0;
    char *host, *port, *default_port = "80";

//    printf("connecting to : %s\n", bt->tracker_url);
//    printf("connecting to tracker: '%s:%s'\n", host, port);
    host = url2host(bt->tracker_url);
    if (!(port = url2port(bt->tracker_url)))
    {
        port = default_port;
    }

    if (1 == bt->net.tracker_connect(&bt->net_udata, host, port, bt->my_ip))
    {
        int rlen;
        char *request, *document, *response;

        __build_tracker_request(bt, &request);
        printf("my ip: %s\n", bt->my_ip);
        printf("requesting peer list: %s\n", request);
        if (
               /*  send http request */
               1
               ==
               bt->net.tracker_send(&bt->net_udata, request, strlen(request)) &&
               /*  receive http response */
               1 == bt->net.tracker_recv(&bt->net_udata, &response, &rlen))
        {
            int bencode_len;

            document = strstr(response, "\r\n\r\n") + 4;
            bencode_len = response + rlen - document;
            bt_client_read_tracker_response(bt, document, bencode_len);
            free(request);
            free(response);
            status = 1;
        }
    }

    free(host);
    if (port != default_port)
        free(port);
    return status;
}

/**
 * 
 * Send request to tracker.
 * Get peer listing.
 * */
int bt_client_connect_to_tracker(
    void *bto
)
{
    bt_client_t *bt = bto;

    assert(bt);
    assert(bt->tracker_url);
    assert(bt->info_hash);
    assert(bt->p_peer_id);
//    asprintf(&request, "GET %s/?info_hash=%s&peer_id=%s", bt->tracker_url,
//             bt->info_hash, bt->peer_id);
    return __get_tracker_request(bt);
}

void bt_client_add_tracker_backup(
    void *bto,
    char *url,
    int url_len
)
{
    printf("backup tracker url: %.*s\n", url_len, url);
}

/*----------------------------------------------------------------------------*/
bt_piece_t *bt_client_get_piece(
    void *bto,
    const int piece_idx
)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get(bt->db, piece_idx);
}

/*----------------------------------------------------------------------------*/

/*
 * peer connections are given this as a callback whenever they want to send
 * information */
static int __FUNC_peerconn_send_to_peer(
    void *bto,
    bt_peer_t * peer,
    void *data,
    const int len
)
{
    bt_client_t *bt = bto;

    return bt->net.peer_send(&bt->net_udata, peer->net_peerid, data, len);
}

static int __FUNC_peercon_pollblock(
    void *bto,
    bt_bitfield_t * peer_bitfield,
    bt_block_t * blk
)
{
    bt_client_t *bt = bto;

//    bt_block_tpwp_piece_block_t req;
    bt_piece_t *pce;

    pce = bt_piecedb_poll_best_from_bitfield(bt->db, peer_bitfield);
    if (pce)
    {
        //    bt_piece_build_request(pce, blk);
        assert(!bt_piece_is_complete(pce));
        assert(!bt_piece_is_fully_requested(pce));
        bt_piece_poll_block_request(pce, blk);
        return 0;
    }
    else
    {
        return -1;
    }
}

/*  return how many we've read */
static int __FUNC_peercon_recv(
    void *bto,
    bt_peer_t * peer,
    char *buf,
    int *len
)
{
    bt_client_t *bt = bto;

//        bt->net.peer_recv(&bt->net_udata, peer->net_peerid, &msg, &len);
    return bt->net.peer_recv_len(&bt->net_udata, peer->net_peerid, buf, len);
}

/**  received a block from a peer
 *
 * @peer : peer received from
 *
 * */
static int __FUNC_peercon_pushblock(
    void *bto,
    bt_peer_t * peer,
    bt_block_t * block,
    void *data
)
{
    bt_client_t *bt = bto;
    bt_piece_t *pce;

    pce = bt_client_get_piece(bt, block->piece_idx);
    assert(pce);
    bt_piece_write_block(pce, NULL, block, data);
//    bt_filedumper_write_block(bt->fd, block, data);
    if (bt_piece_is_complete(pce))
    {
//        printf("COMPLETED %d\n", bt_piece_is_valid(pce));
        /*  send have to all peers */
        if (bt_piece_is_valid(pce))
        {
            int ii;

            __log(bt, NULL,
                  "client,piece downloaded,pieceidx=%d\n",
                  bt_piece_get_idx(pce));
            for (ii = 0; ii < bt->npeers; ii++)
            {
                void *pc;

                pc = bt->peerconnects[ii];
                if (!bt_peerconn_is_active(pc))
                    continue;
                bt_peerconn_send_have(pc, bt_piece_get_idx(pce));
            }
        }

        if (__all_pieces_are_complete(bt))
        {
            bt->am_seeding = 1;
            bt_diskcache_disk_dump(bt->dc);
        }
    }

    __print_pieces_downloaded(bt);
    return 1;
}

/*----------------------------------------------------------------------------*/

static int __have_peer(
    void *bto,
    const char *ip,
    const int port
)
{
    bt_client_t *bt = bto;
    int ii;

    for (ii = 0; ii < bt->npeers; ii++)
    {
        bt_peer_t *peer;

        peer = bt_peerconn_get_peer(bt->peerconnects[ii]);
        if (!strcmp(peer->ip, ip) && atoi(peer->port) == port)
        {
            return 1;
        }
    }
    return 0;
}

/*
 * add the peer;
 * initiate the connection
 *
 * @return bt_peer
 */
bt_peer_t *bt_client_add_peer(
    void *bto,
    const char *peer_id,
    const int peer_id_len,
    const char *ip,
    const int ip_len,
    const int port
)
{
    bt_client_t *bt = bto;
    void *pc;
    bt_peer_t *peer;

    /*  peer is me.. */
    if (!strncmp(ip, bt->my_ip, ip_len) && port == bt->pwp_listen_port)
    {
        return NULL;
    }
    /* prevent dupes.. */
    else if (__have_peer(bt, ip, port))
    {
        return NULL;
    }

    peer = calloc(1, sizeof(bt_peer_t));
    /*  'compact=0'
     *  doesn't use peerids.. */
    if (peer_id)
    {
        asprintf(&peer->peer_id, "00000000000000000000");
    }
    else
    {
        asprintf(&peer->peer_id, "", peer_id_len, peer_id);
    }
    asprintf(&peer->ip, "%.*s", ip_len, ip);
    asprintf(&peer->port, "%d", port);
    pc = bt_peerconn_new();
    bt_peerconn_set_peer(pc, peer);
    bt_peerconn_set_func_getpiece(pc, bt_client_get_piece);
    bt_peerconn_set_func_push_block(pc, __FUNC_peercon_pushblock);
    bt_peerconn_set_func_send(pc, __FUNC_peerconn_send_to_peer);
    bt_peerconn_set_func_recv(pc, __FUNC_peercon_recv);
    bt_peerconn_set_func_pollblock(pc, __FUNC_peercon_pollblock);
    bt_peerconn_set_func_log(pc, __FUNC_peerconn_log);
    bt_peerconn_set_func_disconnect(pc, __FUNC_peerconn_disconnect);
    bt_peerconn_set_func_connect(pc, __FUNC_peerconn_connect);
    bt_peerconn_set_func_get_infohash(pc, __FUNC_peerconn_get_infohash);
    bt_peerconn_set_func_piece_is_complete(pc, __FUNC_peerconn_pieceiscomplete);
    bt_peerconn_set_pieceinfo(pc, &bt->pinfo);
    bt_peerconn_set_isr_udata(pc, bt);
    bt_peerconn_set_pwp_cfg(pc, &bt->pwpcfg);
    bt_peerconn_set_peer_id(pc, bt->p_peer_id);
    printf("adding peer: ip:%.*s port:%d\n", ip_len, ip, port);
    bt->npeers++;
    bt->peerconnects = realloc(bt->peerconnects, sizeof(void *) * bt->npeers);
    bt->peerconnects[bt->npeers - 1] = pc;
    bt_leeching_choker_add_peer(bt->lchoke, pc);
    return peer;
}

int bt_client_remove_peer(
    void *bto,
    const int peerid
)
{
//    bt_client_t *bt = bto;
//    bt->peerconnects[bt->npeers - 1]

//    bt_leeching_choker_add_peer(bt->lchoke, peer);
    return 0;
}

int bt_client_is_done(
    void *bto
)
{
    return 0;
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

/*
 * @return -1 on error */
int bt_client_add_piece(
    void *bto,
    const char *sha1
)
{
    bt_client_t *bt = bto;

//    printf("got a piece '%.*s'\n", 20, sha1);
    bt_piecedb_add(bt->db, sha1);
    bt->pinfo.npieces = bt_piecedb_get_length(bt->db);
    return 1;
}

void bt_client_add_pieces(
    void *bto,
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
//            printf("%d\n", prog);
            bt_client_add_piece(bto, pieces);
            pieces += 20;
        }
    }
}

#if 1
/**
 * Add this file to the bittorrent client
 *
 * @id id of bt client
 * @fname file name
 * @fname_len length of fname
 * @flen length in bytes of the file */
int bt_client_add_file(
    void *bto,
    const char *fname,
    const int fname_len,
    const int flen
)
{
    bt_client_t *bt = bto;
    char *path = NULL;

    printf("adding file: %.*s (%d bytes) flen:%d\n",
           fname_len, fname, flen, fname_len);
//    bt->tot_file_size += flen;
    bt_piecedb_set_tot_file_size(bt->db,
                                 bt_piecedb_get_tot_file_size(bt->db) + flen);
    asprintf(&path, "%.*s", fname_len, fname);
    bt_filedumper_add_file(bt->fd, path, flen);
    return 0;
}
#endif

/*----------------------------------------------------------------------------*/

static void *__netpeerid_to_peerconn(
    bt_client_t * bt,
    const int netpeerid
)
{
    int ii;

    for (ii = 0; ii < bt->npeers; ii++)
    {
        void *pc = bt->peerconnects[ii];
        bt_peer_t *peer;

//        if (!bt_peerconn_is_active(pc))
//            continue;
        peer = bt_peerconn_get_peer(pc);
        if (peer->net_peerid == netpeerid)
            return pc;
    }

    assert(FALSE);
    return NULL;
}

/**
 * Take this message and process it on the PeerConnection side
 * */
static int __process_peer_msg(
    void *bto,
    const int netpeerid
)
{
    bt_client_t *bt = bto;
    void *pc;

    pc = __netpeerid_to_peerconn(bt, netpeerid);
//    printf("available data from peer: %d\n", netpeerid);
    bt_peerconn_process_msg(pc);

    return 1;
}

static void __process_peer_connect(
    void *bto,
    const int netpeerid,
    char *ip,
    const int port
)
{
    bt_client_t *bt = bto;
    bt_peer_t *peer;
    void *pc;

    printf("new connection!! %d\n\n\n\n", netpeerid);
    peer = bt_client_add_peer(bt, NULL, 0, ip, strlen(ip), port);
    peer->net_peerid = netpeerid;
    pc = __netpeerid_to_peerconn(bt, netpeerid);
//    bt_peerconn_set_active(pc, TRUE);
//    pc = __netpeerid_to_peerconn(bt, netpeerid);
}

#include <sys/time.h>
#include <sys/resource.h>

static void __log_process_info(
    bt_client_t * bt
)
{
#if 0
    struct rusage
    {
        struct timeval ru_utime;        /* user time used */
        struct timeval ru_stime;        /* system time used */
        long ru_maxrss;         /* maximum resident set size */
        long ru_ixrss;          /* integral shared memory size */
        long ru_idrss;          /* integral unshared data size */
        long ru_isrss;          /* integral unshared stack size */
        long ru_minflt;         /* page reclaims */
        long ru_majflt;         /* page faults */
        long ru_nswap;          /* swaps */
        long ru_inblock;        /* block input operations */
        long ru_oublock;        /* block output operations */
        long ru_msgsnd;         /* messages sent */
        long ru_msgrcv;         /* messages received */
        long ru_nsignals;       /* signals received */
        long ru_nvcsw;          /* voluntary context switches */
        long ru_nivcsw;         /* involuntary context switches */
    };
#endif
    struct rusage stats;
    static long int last_run = 0;
    struct timeval tv;

#define SECONDS_SINCE_LAST_LOG 1
    gettimeofday(&tv, NULL);
    /*  run every n seconds */
    if (0 == last_run)
    {
        last_run = tv.tv_usec;
    }
    else
    {
        unsigned int diff = abs(last_run - tv.tv_usec);

        if (diff >= SECONDS_SINCE_LAST_LOG)
            return;
        last_run = tv.tv_usec;
    }

    getrusage(RUSAGE_SELF, &stats);
    __log(bt, NULL,
          "stats, ru_utime=%d, ru_stime=%d, ru_maxrss=%d, ru_ixrss=%d, ru_idrss=%d, ru_isrss=%d, ru_minflt=%d, ru_majflt=%d, ru_nswap=%d, ru_inblock=%d, ru_oublock=%d, ru_msgsnd=%d, ru_msgrcv=%d, ru_nsignals=%d, ru_nvcsw=%d, ru_nivcsw=%d",
          stats.ru_utime, stats.ru_stime, stats.ru_maxrss,
          stats.ru_ixrss, stats.ru_idrss, stats.ru_isrss,
          stats.ru_minflt, stats.ru_majflt,
          stats.ru_nswap, stats.ru_inblock,
          stats.ru_oublock, stats.ru_msgsnd,
          stats.ru_msgrcv, stats.ru_nsignals, stats.ru_nvcsw, stats.ru_nivcsw);
}

/**
 * @return 0 on failure, 1 otherwise */
static int __xconnect_to_peer(
    bt_client_t * bt,
    void *pc,
    bt_peer_t * peer
)
{
    /* the remote peer will have always send a handshake */
    if (!bt->net.peer_connect)
    {
        return 0;
    }

    printf("connecting to peer (%s:%s)\n", peer->ip, peer->port);
    if (0 ==
        bt->net.peer_connect(&bt->net_udata, peer->ip,
                             peer->port, &peer->net_peerid))
    {
        printf("failed\n");
        return 0;
    }

    printf("handshaking with peer: %d...\n", peer->net_peerid);
    bt_peerconn_set_active(pc, TRUE);
    if (0 == bt_peerconn_send_handshake(pc, bt->info_hash, bt->p_peer_id))
    {
        bt_peerconn_set_active(pc, FALSE);
        return 0;
    }

    return 1;
}

/**
 * Run peerconnection step.
 * Ensure we are connected to our assigned peers.
 * */
static void __peerconn_step(
    bt_client_t * bt,
    int ii
)
{
    void *pc;
    bt_peer_t *peer;

    pc = bt->peerconnects[ii];
    peer = bt_peerconn_get_peer(pc);

#if 0

    if (bt->am_seeding)
    {
        if (!bt_peerconn_is_active(pc))
        {
            return;
        }
    }
    else
    {
        if (!bt_peerconn_is_active(pc))
            // && peer is not rubbish
        {
            if (0 == __connect_to_peer(bt, pc, peer))
                return;
        }
    }
#endif

    bt_peerconn_step(pc);
}

#include <time.h>

/**
 * */
int bt_client_step(
    void *bto
)
{
    bt_client_t *bt = bto;
    int ii;
    time_t seconds;

    __log_process_info(bt);
    seconds = time(NULL);

    /*  shutdown if we are setup to not seed */
    if (1 == bt->am_seeding && 1 == bt->o_shutdown_when_complete)
        return 1;

    /*  perform tracker request to get new peers */
    if (bt->last_tracker_request + bt->cfg.tracker_scrape_interval < seconds)
    {
        bt->last_tracker_request = seconds;
        __get_tracker_request(bt);
    }

    /*  poll data from peer pwp connections */
    bt->net.peers_poll(&bt->net_udata,
                       bt->cfg.select_timeout_msec,
                       __process_peer_msg, __process_peer_connect, bt);

    /*  run each peer connection step */
    for (ii = 0; ii < bt->npeers; ii++)
    {
        __peerconn_step(bt, ii);
    }

//    if (__all_pieces_are_complete(bt))
//        return 0;
    return 0;
}

void bt_client_go(
    void *bto
)
{
    bt_client_t *bt = bto;
    int ii;

    bt->net.peer_listen_open(&bt->net_udata, bt->pwp_listen_port);

    while (1)
    {
        if (1 == bt_client_step(bt))
            break;
    }

    printf("download is done!\n");
    int fd;

    fd = open("piecedump", O_CREAT | O_WRONLY);
    for (ii = 0; ii < bt_piecedb_get_length(bt->db); ii++)
    {
        bt_piece_t *pce = bt_piecedb_get(bt->db, ii);

        write(fd, bt_piece_get_data(pce), bt_piece_get_size(pce));
    }

    close(fd);
}

int bt_client_release(
    void *bto
)
{
    //FIXME_STUB;

    return 1;
}

/*
 The size of a piece is determined by the publisher of the torrent. A good recommendation is to use a piece size so that the metainfo file does not exceed 70 kilobytes.
 */

// number_of_blocks = (fixed_piece_size / fixed_block_size)
//                       + !!(fixed_piece_size % fixed_block_size)

/*----------------------------------------------------------------------------*/
void bt_client_set_max_peer_connections(
    void *bto,
    const int npeers
)
{
    bt_client_t *bt = bto;

    bt->cfg.max_peer_connections = npeers;
}

void bt_client_set_select_timeout_msec(
    void *bto,
    const int val
)
{
    bt_client_t *bt = bto;

    bt->cfg.select_timeout_msec = val;
}

void bt_client_set_max_cache_mem(
    void *bto,
    const int val
)
{
    bt_client_t *bt = bto;

    bt->cfg.max_cache_mem = val;
}

/*  set the number of completed peers */
void bt_client_set_num_complete_peers(
    void *bto,
    const int npeers
)
{
    bt_client_t *bt = bto;

    bt->ncomplete_peers = npeers;
}

void bt_client_set_failed(
    void *bto,
    const char *reason
)
{

}

void bt_client_set_interval(
    void *bto,
    int interval
)
{

}

void bt_client_set_info_hash(
    void *bto,
    unsigned char *info_hash
)
{
    int ii;
    bt_client_t *bt = bto;

    bt->info_hash = info_hash;
    printf("Info hash.....: ");
    for (ii = 0; ii < 20; ii++)
        printf("%02x", info_hash[ii]);
    printf("\n");
#if 0
    for (ii = 0; ii < bt->npeers; ii++)
    {
        void *pc = bt->peerconnects[ii];
    }
#endif
}

void bt_client_set_peer_id(
    void *bto,
    char *peer_id
)
{
    bt_client_t *bt = bto;
    int ii;

    bt->p_peer_id = peer_id;
    for (ii = 0; ii < bt->npeers; ii++)
    {
        bt_peerconn_set_peer_id(bt->peerconnects[ii], peer_id);
    }
}

char *bt_client_get_peer_id(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt->p_peer_id;
}

/*
 * How many pieces are there of this file?
 * Note: bt_read_torrent_file sets this */
void bt_client_set_npieces(
    void *bto,
    int npieces
)
{
    assert(FALSE);
// fixed_piece_size = size_of_torrent / number_of_pieces
    bt_client_t *bt = bto;

    bt->pinfo.npieces = npieces;
}

void bt_client_set_tracker_url(
    void *bto,
    const char *url,
    const int len
)
{
    bt_client_t *bt = bto;

    bt->tracker_url = calloc(1, sizeof(char) * (len + 1));
    strncpy(bt->tracker_url, url, len);
}

void bt_client_set_path(
    void *bto,
    const char *path
)
{
    bt_client_t *bt = bto;

    bt->path = strdup(path);
    bt_filedumper_set_path(bt->fd, bt->path);
}

void bt_client_set_piece_length(
    void *bto,
    const int len
)
{
    bt_client_t *bt = bto;

    printf("setting piece length: %d\n", len);
    bt->pinfo.piece_len = len;
    bt_piecedb_set_piece_length(bt->db, len);
    bt_filedumper_set_piece_length(bt->fd, len);
    bt_diskcache_set_size(bt->dc, len);
}

void bt_client_set_logging(
    void *bto,
    void *udata,
    func_log_f func
)
{
    bt_client_t *bt = bto;

    bt->func_log = func;
    bt->log_udata = udata;
}

void bt_client_set_net_funcs(
    void *bto,
    bt_net_funcs_t * net
)
{
    bt_client_t *bt = bto;

    memcpy(&bt->net, net, sizeof(bt_net_funcs_t));
}

/**
 *
 * @return 1 on success, 255 if the option doesn't exist, 0 otherwise
 * */
void bt_client_set_opt_shutdown_when_completed(
    void *bto,
    const int flag
)
{
    bt_client_t *bt = bto;

    bt->o_shutdown_when_complete = flag;
}

int bt_client_set_opt(
    void *bto,
    const char *key,
    const char *val
)
{
    bt_client_t *bt = bto;

    if (!strcmp(key, "pwp_listen_port"))
    {
        bt->pwp_listen_port = atoi(val);
        printf("set pwp_listen_port=%d\n", bt->pwp_listen_port);
        return 1;
    }

    return 255;
}

/*----------------------------------------------------------------------------*/

int bt_client_get_num_peers(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt->npeers;
}

int bt_client_get_num_pieces(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get_length(bt->db);
}

char *bt_client_get_tracker_url(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt->tracker_url;
}

int bt_client_get_total_file_size(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt_piecedb_get_tot_file_size(bt->db);
}

char *bt_client_get_info_hash(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt->info_hash;
}

char *bt_client_get_fail_reason(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt->fail_reason;
}

int bt_client_get_interval(
    void *bto
)
{
    bt_client_t *bt = bto;

    return bt->interval;
}

int bt_client_get_nbytes_downloaded(
    void *bto
)
{
    //FIXME_STUB;
    return 0;
}

int bt_client_is_failed(
    void *bto
)
{

    return 0;
}
