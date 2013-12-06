
typedef void* bt_dm_t;

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

/**
 * @return 0 on error */
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

typedef struct {
    /* Peer stats */
    int failed_connection;
    int connected;
    int peers;
    int choked;
    int choking;
    int download_rate;
    int upload_rate;
} bt_dm_stats_t;

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
    /* TODO: change to const? */
    int idx;

} bt_piece_t;

typedef struct {
//    void* (*poll_best_from_bitfield)(void * db, void * bf_possibles);
    void* (*get_piece)(void *db, const unsigned int piece_idx);
    void* (*get_sparsecounter)(void *db);
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
    /**
     * Poll best piece from peer
     * @param r random object
     * @param peer Best piece in context of this peer
     * @return idx of piece which is best; otherwise -1 */
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
     * @param me 
     * @param nethandle Pointer available for the callee to identify the peer
     * @param udata Memory to be used for connection. Callee's responsibility to alloc memory.
     * @param host Hostname
     * @param port Host's port
     * @param func_process_connection Callback for sucessful connections.
     * @param func_connection_failed Callback for failed connections.
     * @return 1 if successful; otherwise 0 */
    int (*peer_connect) (void* me,
                        void **udata,
                        void **nethandle,
                        const char *host, const int port,

       /**
        * We've received data from the peer.
        * Announce this to the cb_ctx.
        *
        * @param me The caller
        * @param nethandle Peer ID 
        * @param buf Buffer containing data
        * @param len Bytes available in buffer
        */
        int (*func_process_data) (void *me,
        void* nethandle,
        const unsigned char* buf,
        unsigned int len),

       /**
        * We've determined that we are now connected.
        * Announce the connection to the caller.
        *
        * @param me The caller
        * @param nethandle Peer ID 
        * @param ip The IP that connected to us
        * @return 0 on failure
        */
        int (*func_process_connection) (
            void *me,
            void *nethandle,
            char *ip,
            int port),

       /**
        * The connection attempt failed.
        * Announce the failed connection to the caller.
        *
        * @param me The caller
        * @param nethandle Peer ID */
        void (*func_connection_failed) (void *, void* nethandle));

    /**
     * Send data to peer
     *
     * @param me 
     * @param nethandle The peer's network ID
     * @param send_data Data to be sent
     * @param len Length of data to be sent */
    int (*peer_send) (void* me,
                      void **udata,
                      void* nethandle,
                      const unsigned char *send_data, const int len);

    /**
     * Drop the connection for this peer
     * @return 1 on success, otherwise 0 */
    int (*peer_disconnect) (void* me, void **udata, void* nethandle);

    /**
     * Waits until lock is released, and then runs callback.
     * Creates lock when lock is NULL
     * @param me The caller
     * @param lock Lock to be used. Will be initialised if NULL
     * @param udata Argument for callback
     * @param cb Callback
     * @return result of callback */
    void* (*call_exclusively)(void* me, void* cb_ctx, void **lock, void* udata,
            void* (*cb)(void* me, void* udata));

    func_log_f log;

} bt_dm_cbs_t;

int bt_dm_dispatch_from_buffer(
        void *bto,
        void *peer_nethandle,
        const unsigned char* buf,
        unsigned int len);

void *bt_peer_get_nethandle(void* pr);

void bt_dm_peer_connect_fail(void *bto, void* nethandle);

int bt_dm_peer_connect(void *bto, void* nethandle, char *ip, const int port);

void bt_dm_set_piece_db(bt_dm_t* me_, bt_piecedb_i* ipdb, void* piecedb);

void *bt_dm_get_piecedb(bt_dm_t* me_);

void *bt_dm_new();

int bt_dm_get_num_peers(bt_dm_t* me_);

int bt_dm_get_total_file_size(bt_dm_t* me_);

char *bt_dm_get_fail_reason(bt_dm_t* me_);

int bt_dm_get_nbytes_downloaded(bt_dm_t* me_);

int bt_dm_is_failed(bt_dm_t* me_);

void bt_dm_periodic(bt_dm_t* me_, bt_dm_stats_t *stats);

void *bt_dm_add_peer(bt_dm_t* me_,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port,
                              void* nethandle);

void* bt_dm_get_config(bt_dm_t* me_);

char *bt_generate_peer_id();

void bt_dm_check_pieces(bt_dm_t* me_);

int bt_piece_write_block(
    bt_piece_t *pceo,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata,
    void* peer
);

