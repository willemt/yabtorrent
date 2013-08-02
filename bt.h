
//#include "bt_block_readwriter_i.h"

typedef int (
    *func_write_block_f
)   (
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
);

typedef void *(
    *func_read_block_f
)    (
    void *udata,
    void *caller,
    const bt_block_t * blk
);

typedef void *(
    *func_add_file_f
)    (
    void *caller,
    const char *fname,
    const int size
);


#ifndef HAVE_FUNC_LOG
#define HAVE_FUNC_LOG
typedef void (
    *func_log_f
)    (
    void *udata,
    void *src,
    const char *buf,
    ...
);
#endif

typedef struct
{
    func_write_block_f write_block;

    func_read_block_f read_block;

    /*  release this block from the holder of it */
//    func_giveup_block_f giveup_block;
} bt_blockrw_i;

/*  piece info
 *  this is how this torrent has */
typedef struct
{
    /* a string containing the 20 byte sha1 of every file, concatenated.
     * (from protocol)
     * sha1 hash = 20 bytes */
    char *pieces_hash;
    /* the length of a piece (from protocol) */
    int piece_len;
    /* number of pieces (from protocol) */
    int npieces;
} bt_piece_info_t;

#if 1
/** cfg */
typedef struct
{
    int select_timeout_msec;
    int max_peer_connections;
    int max_active_peers;
    int max_cache_mem;
//    int tracker_scrape_interval;
    /*  don't seed, shutdown when complete */
    int o_shutdown_when_complete;
    /*  the size of the piece, etc */
    bt_piece_info_t pinfo;
    /*  how many seconds between tracker scrapes */
//    int o_tracker_scrape_interval;
    /* listen for pwp messages on this port */
    int pwp_listen_port;
    /*  this holds my IP. I figure it out */
    char my_ip[32];
    /* sha1 hash of the info_hash */
    char *info_hash;
    /* 20-byte self-designated ID of the peer */
    char *p_peer_id;

//    char *tracker_url;
} bt_client_cfg_t;
#endif

typedef struct
{
    /**
     * Connect to the peer
     * @param udata use this memory for the connection. It is up to the callee to alloc memory.
     * @param host the hostname
     * @param port the host's port
     * @param nethandle pointer available for the callee to identify the peer
     * @param func_process_connection Callback for connections. */
    int (*peer_connect) (void **udata,
                         const char *host, const int port, void **nethandle,
                        void (*func_process_connection) (
                            void *, void* nethandle,
                            char *ip, int iplen));

    /**
     * Send data to peer
     *
     * @param nethandle The peer's network ID
     * @param send_data Data to be sent
     * @param len Length of data to be sent */
    int (*peer_send) (void **udata,
                      void* nethandle,
                      const unsigned char *send_data, const int len);

    /**
     * Drop the connection for this peer
     */
    int (*peer_disconnect) (void **udata, void* nethandle);

    /**
     * Call the network stack and receive packets for our peers
     */
    int (*peers_poll) (void **udata,
                       const int msec_timeout,
                       /**
                        * We've received data from the peer.
                        * Announce this to the caller.
                        *
                        * @param caller The caller
                        * @param nethandle Peer ID 
                        * @param buf Buffer containing data
                        * @param len Bytes available in buffer
                        */
                       int (*func_process) (void *caller,
                                            void* nethandle,
                                            const unsigned char* buf,
                                            unsigned int len),
                       /**
                        * We've determined that we are now connected.
                        * Announce the connection to the caller.
                        *
                        * @param caller The caller
                        * @param nethandle Peer ID 
                        */
                       void (*func_process_connection) (void *,
                                                        void* nethandle,
                                                        char *ip,
                                                        int),
                       void *data);

    int (*peer_listen_open) (void **udata, const int port);

} bt_client_funcs_t;

/*  bittorrent piece */
typedef struct
{
    /* index on 'bit stream' */
    const int idx;

} bt_piece_t;

typedef struct {
    void* (*poll_best_from_bitfield)(void * db, void * bf_possibles);
    void* (*get_piece)(void *db, const unsigned int piece_idx);
} bt_piecedb_i;

void bt_client_set_piecedb(void* bto, bt_piecedb_i* ipdb, void* piecedb);

char *bt_generate_peer_id();

void *bt_client_new();

int bt_client_get_num_peers(void *bto);

int bt_client_get_num_pieces(void *bto);

int bt_client_get_total_file_size(void *bto);

char *bt_client_get_fail_reason(void *bto);

int bt_client_get_nbytes_downloaded(void *bto);

int bt_client_is_failed(void *bto);

void *bt_client_get_piecedb(void *bto);

void *bt_client_add_peer(void *bto,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port);

void* bt_client_get_config(void *bto);

char *str2sha1hash(const char *str, int len);

