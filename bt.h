
//#include "bt_block_readwriter_i.h"

typedef int (
    *func_object_get_int_f
)   (
    void *
);

#ifndef HAVE_FUNC_GET_INT
#define HAVE_FUNC_GET_INT
typedef int (
    *func_get_int_f
)   (
    void *,
    void *pr
);
#endif

typedef int (
    *func_get_int_const_f
)   (
    const void *,
    const void *pr
);

#if 0
enum {
    BT_WRITEBLOCK_NONE,
    /*  commit this block as much as possible */
    BT_WRITEBLOCK_FLUSH
};
#endif

typedef int (
    *func_flush_block_f
)   (
    void *udata,
    void *caller,
    const bt_block_t * blk
);

typedef int (
    *func_write_block_f
)   (
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
//    int opt
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
    func_flush_block_f flush_block;
} bt_blockrw_i;

/**
 * Piece info
 * This is how this torrent has */
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

typedef struct {
    void* (*new)(int npieces);
    /* add a new peer to the selector */
    void (*add_peer)(void *r, void *peer);
    /* remove a peer from the selector */
    void (*remove_peer)(void *r, void *peer);
    /* register this piece as something we have */
    void (*have_piece)(void *r, int piece_idx);
    /* register this piece as being available from the peer */
    void (*peer_have_piece)(void *r, void* peer, int piece_idx);
    /* give this piece back to the selector */
    void (*peer_giveback_piece)(void *r, void* peer, int piece_idx);
    /* poll a piece */
    int (*poll_piece)(void* r, const void* peer);
    /* get number of peers */
    int (*get_npeers)(void* r);
    /* get number of pieces */
    int (*get_npieces)(void* r);
} bt_pieceselector_i;

typedef struct
{
    /**
     * Connect to the peer
     * @param caller 
     * @param nethandle Pointer available for the callee to identify the peer
     * @param udata Memory to be used for connection. Callee's responsibility to alloc memory.
     * @param host Hostname
     * @param port Host's port
     * @param func_process_connection Callback for sucessful connections.
     * @param func_connection_failed Callback for failed connections.
     * @return 1 if successful; otherwise 0 */
    int (*peer_connect) (void* caller,
                        void **udata,
                        void **nethandle,
                        const char *host, const int port,

       /**
        * We've received data from the peer.
        * Announce this to the caller.
        *
        * @param caller The caller
        * @param nethandle Peer ID 
        * @param buf Buffer containing data
        * @param len Bytes available in buffer
        */
        int (*func_process_data) (void *caller,
        void* nethandle,
        const unsigned char* buf,
        unsigned int len),

       /**
        * We've determined that we are now connected.
        * Announce the connection to the caller.
        *
        * @param caller The caller
        * @param nethandle Peer ID 
        * @param ip The IP that connected to us
        */
        void (*func_process_connection) (
            void *caller,
            void *nethandle,
            char *ip,
            int port),

       /**
        * The connection attempt failed.
        * Announce the failed connection to the caller.
        *
        * @param caller The caller
        * @param nethandle Peer ID 
        */
        void (*func_connection_failed) (void *, void* nethandle));

    /**
     * Send data to peer
     *
     * @param caller 
     * @param nethandle The peer's network ID
     * @param send_data Data to be sent
     * @param len Length of data to be sent */
    int (*peer_send) (void* caller,
                      void **udata,
                      void* nethandle,
                      const unsigned char *send_data, const int len);

    /**
     * Drop the connection for this peer
     */
    int (*peer_disconnect) (void* caller,void **udata, void* nethandle);

} bt_client_funcs_t;

int bt_client_dispatch_from_buffer(
        void *bto,
        void *peer_nethandle,
        const unsigned char* buf,
        unsigned int len);

void *bt_peer_get_nethandle(void* pr);

void bt_client_peer_connect_fail(void *bto, void* nethandle);

void bt_client_peer_connect(void *bto, void* nethandle, char *ip, const int port);

void bt_client_set_piece_db(void* bto, bt_piecedb_i* ipdb, void* piecedb);

void bt_client_set_piece_db(void* bto, bt_piecedb_i* ipdb, void* piecedb);

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
                              const char *ip, const int ip_len, const int port,
                              void* nethandle);

void* bt_client_get_config(void *bto);

char *str2sha1hash(const char *str, int len);

