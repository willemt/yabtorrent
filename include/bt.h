
/**
 * Copyright (c) 2011, Willem-Hendrik Thiart
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file. 
 *
 * @file
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 */

#ifndef BT_H_
#define BT_H_

#ifndef HAVE_BT_BLOCK_T
#define HAVE_BT_BLOCK_T
typedef struct
{
    unsigned int piece_idx;
    unsigned int offset;
    unsigned int len;
} bt_block_t;
#endif

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


typedef struct
{
    void* in;
} bt_diskcache_t;

typedef void* bt_dm_t;

/**
 * Peer statistics */
typedef struct {
    int drate;
    int urate;
    int choked;
    int choking;
    int connected;
    int failed_connection;
} bt_dm_peer_stats_t;

typedef struct {
    /* individual peer stats */
    bt_dm_peer_stats_t *peers;

    /* number of peers in array */
    int npeers;

    /* size of array */
    int npeers_size;
} bt_dm_stats_t;

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
     * sha1 hash = 20 bytes */
    char *pieces_hash;

    /* the length of a piece (from protocol) */
    int piece_len;

    /* number of pieces (from protocol) */
    int npieces;
} bt_piece_info_t;

/**
 * Bittorrent piece */
typedef struct
{
    /* TODO: change to const? */
    int idx;
} bt_piece_t;

typedef struct {
    void* (*get_piece)(void *db, const unsigned int piece_idx);
    void* (*get_sparsecounter)(void *db);
} bt_piecedb_i;

typedef struct {
    void* (*new)(int npieces);

    /**
     * Add a new peer to the selector */
    void (*add_peer)(void *r, void *peer);

    /**
     * Remove a peer from the selector */
    void (*remove_peer)(void *r, void *peer);

    /**
     * Register this piece as something we have */
    void (*have_piece)(void *r, int piece_idx);

    /* 
     * Register this piece as being available from the peer */
    void (*peer_have_piece)(void *r, void* peer, int piece_idx);

    /* 
     * Give this piece back to the selector */
    void (*peer_giveback_piece)(void *r, void* peer, int piece_idx);

    /**
     * Poll best piece from peer
     * @param r random object
     * @param peer Best piece in context of this peer
     * @return idx of piece which is best; otherwise -1 */
    int (*poll_piece)(void* r, const void* peer);

    /**
     * @return get number of peers */
    int (*get_npeers)(void* r);

    /*
     * Get number of pieces */
    int (*get_npieces)(void* r);

} bt_pieceselector_i;

#define BT_HANDSHAKER_DISPATCH_SUCCESS 1
#define BT_HANDSHAKER_DISPATCH_REMAINING 0
#define BT_HANDSHAKER_DISPATCH_ERROR -1

typedef struct
{
    /**
     * Connect to the peer
     * @param me 
     * @param conn_ctx Pointer available for the callee to identify the peer
     * @param udata Memory to be used for connection. Callee's responsibility to alloc memory.
     * @param host Hostname
     * @param port Host's port
     * @param func_process_connection Callback for sucessful connections.
     * @param func_connection_failed Callback for failed connections.
     * @return 1 if successful; otherwise 0 */
    int (*peer_connect) (void* me,
                        void **udata,
                        void **conn_ctx,
                        const char *host, const int port,

       /**
        * We've received data from the peer.
        * Announce this to the cb_ctx.
        *
        * @param me The caller
        * @param conn_ctx Peer ID 
        * @param buf Buffer containing data
        * @param len Bytes available in buffer
        */
        int (*func_process_data) (void *me,
        void* conn_ctx,
        const unsigned char* buf,
        unsigned int len),

       /**
        * We've determined that we are now connected.
        * Announce the connection to the caller.
        *
        * @param me The caller
        * @param conn_ctx Peer ID 
        * @param ip The IP that connected to us
        * @return 0 on failure
        */
        int (*func_process_connection) (
            void *me,
            void *conn_ctx,
            char *ip,
            int port),

       /**
        * The connection attempt failed.
        * Announce the failed connection to the caller.
        *
        * @param me The caller
        * @param conn_ctx Peer ID */
        void (*func_connection_failed) (void *, void* conn_ctx));

    /**
     * Send data to peer
     *
     * @param me 
     * @param conn_ctx The peer's network ID
     * @param send_data Data to be sent
     * @param len Length of data to be sent */
    int (*peer_send) (void* me,
                      void **udata,
                      void* conn_ctx,
                      const unsigned char *send_data, const int len);

    /**
     * Drop the connection for this peer
     * @return 1 on success, otherwise 0 */
    int (*peer_disconnect) (void* me, void **udata, void* conn_ctx);

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

    /**
     * @return newly initialised handshaker */
    void* (*handshaker_new)(unsigned char* expected_info_hash, unsigned char* mypeerid);

    /**
     * deallocate handshaker */
    void (*handshaker_release)(void* hs);

    /**
     *  Receive handshake from other end
     *  Disconnect on any errors
     *  @return 1 succesful handshake; 0 unfinished reading; -1 bad handshake */
    int (*handshaker_dispatch_from_buffer)(void* me_,
            const unsigned char** buf,
            unsigned int* len);

    /**
     * Send the handshake
     * @return 0 on failure; 1 otherwise */
    int (*send_handshake)(
        void* callee,
        void* udata,
        int (*send)(void *callee,
            const void *udata,
            const void *send_data,
            const int len),
        char* expected_ih,
        char* my_pi);

    /**
     * Called when a handshake is sucessful
     * @return newly initialised handshaker */
    void (*handshake_success)(void* callee, void* udata, void* pc, void* pnethandle);

    void* (*msghandler_new)(void* callee, void* pc);
} bt_dm_cbs_t;

/**
 * Initiliase the bittorrent client
 * bt_dm uses the mediator pattern to manage the bittorrent download
 * @return download manager on sucess; otherwise NULL */
void *bt_dm_new();

/**
 * Release all memory used by the client
 * Close all peer connections */
int bt_dm_release(bt_dm_t* me_);

/**
 * Connect a peer to the torrent
 *
 * Don't connect a peer if it hasn't been added 
 *
 * @pararm conn_ctx Context for this peer
 * @param ip IP that the peer is from
 * @param port Port that this peer is on
 * @return 0 on error */
int bt_dm_peer_connect(void *bto, void* conn_ctx, char *ip, const int port);

/**
 * Called when a connection has failed.  */
void bt_dm_peer_connect_fail(void *bto, void* conn_ctx);

/**
 * Take this PWP message and process it on the Peer Connection side
 * @return 1 on sucess; 0 otherwise */
int bt_dm_dispatch_from_buffer(
        void *bto,
        void *peer_conn_ctx,
        const unsigned char* buf,
        unsigned int len);

/**
 * Add a peer. Initiate connection with the peer if conn_ctx is NULL
 *
 * Don't add peer twice
 * Don't add myself as peer
 *
 * @param conn_mem Memory that is used for the peer connection. If NULL the
 *                 connection will allocate it's own memory
 * @return the newly initialised peer; NULL on errors */
void *bt_dm_add_peer(bt_dm_t* me_,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port,
                              void* conn_ctx,
                              void* conn_mem);

/**
 * Remove the peer
 * Disconnect the peer if needed
 * @param peer The peer to delete
 * @return 1 on sucess; otherwise 0 */
int bt_dm_remove_peer(bt_dm_t* me_, void* peer);

/**
 * Process events that are dependent on time passing
 * @param stats Collect download/upload statistics. */
void bt_dm_periodic(bt_dm_t* me_, bt_dm_stats_t *stats);

/**
 * Set callback functions
 * 
 * If funcs.handshaker_dispatch_from_buffer is not set:
 *  No handshaking will occur. This is useful for client variants which
 *  establish a handshake earlier at a higher level.
 *
 * @param funcs Structure containing callbacks
 * @param cb_ctx Context included with each callback */
void bt_dm_set_cbs(bt_dm_t* me_, bt_dm_cbs_t * funcs, void* cb_ctx);

/**
 * @return number of peers this client is involved with */
int bt_dm_get_num_peers(bt_dm_t* me_);

/**
 * @return piece database object */
void *bt_dm_get_piecedb(bt_dm_t* me_);

/**
 * Set piece database object
 * @param ipdb Functions implemented by piece database
 * @param piece_db The piece database passed to piece database functions */
void bt_dm_set_piece_db(bt_dm_t* me_, bt_piecedb_i* ipdb, void* piece_db);

/**
 * Scan over downloaded pieces. Assess whether the pieces are complete. */
void bt_dm_check_pieces(bt_dm_t* me_);

/**
 * @return current configuration */
void* bt_dm_get_config(bt_dm_t* me_);

/**
 * Set the current piece selector
 * This allows us to use dependency injection to de-couple the
 * implementation of the piece selector from bt_dm
 * @param ips Struct of function pointers for piece selector operation
 * @param piece_selector Selector instance. If NULL we call the constructor. */
void bt_dm_set_piece_selector(bt_dm_t* me_, bt_pieceselector_i* ips, void* piece_selector);

void *bt_peer_get_conn_ctx(void* pr);

/**
 * @return random Peer ID */
char *bt_generate_peer_id();

/**
 * @return number of jobs outstanding */
int bt_dm_get_jobs(bt_dm_t* me_);

#endif /* BT_H_ */
