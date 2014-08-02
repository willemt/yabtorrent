#ifndef PWP_CONNECTION_H
#define PWP_CONNECTION_H

typedef void* pwp_conn_t;

#ifndef HAVE_BT_BLOCK_T
#define HAVE_BT_BLOCK_T
typedef struct
{
    unsigned int piece_idx;
    unsigned int offset;
    unsigned int len;
} bt_block_t;
#endif

typedef void *(*func_getpiece_f)(
        void *udata,
        unsigned int piece);

typedef void (*func_write_block_to_stream_f)(
        void *udata,
        bt_block_t *blk,
        char **msg);

#ifndef HAVE_FUNC_LOG
#define HAVE_FUNC_LOG
typedef void (
    *func_log_f
)    (
    void *udata,
    void *src,
//    bt_peer_t * peer,
    const char *buf,
    ...
);
#endif

typedef int (
    *func_pollblock_f
)   (
    void *udata,
    void *peer
);

typedef int (
    *func_pushblock_f
)   (
    void *udata,
    void *peer,
    bt_block_t *block,
    const void *data
);

typedef int (
    *func_send_f
)   (
    void *udata,
    const void *peer,
    const void *send_data,
    const int len
);

typedef int (
    *func_disconnect_f
)   (
    void *udata,
    void *peer,
    char *reason
);

typedef void (
    *func_peergiveblockback_f
)   (
    void *udata,
    void *peer,
    bt_block_t * blk
);

typedef void (
    *func_peerpiece_f
)   (
    void *udata,
    void *peer,
    int piece
);

typedef int (
    *func_lock_f
)   (
    void *udata,
    void **lock
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

//#define PC_NONE ((unsigned int)0)
#define PC_HANDSHAKE_SENT ((unsigned int)1<<0)
#define PC_HANDSHAKE_RECEIVED ((unsigned int)1<<1)
#define PC_DISCONNECTED ((unsigned int)1<<2)
#define PC_BITFIELD_RECEIVED ((unsigned int)1<<3)
/*  connected to peer */
#define PC_CONNECTED ((unsigned int)1<<4)
/*  we can't communicate with the peer */
#define PC_UNCONTACTABLE_PEER ((unsigned int)1<<5)
#define PC_IM_CHOKING ((unsigned int)1<<6)
#define PC_IM_INTERESTED ((unsigned int)1<<7)
#define PC_PEER_CHOKING ((unsigned int)1<<8)
#define PC_PEER_INTERESTED ((unsigned int)1<<9)
#define PC_FAILED_CONNECTION ((unsigned int)1<<10)

typedef enum
{
    PWP_MSGTYPE_CHOKE = 0,
    PWP_MSGTYPE_UNCHOKE = 1,
    PWP_MSGTYPE_INTERESTED = 2,
    PWP_MSGTYPE_UNINTERESTED = 3,
    PWP_MSGTYPE_HAVE = 4,
    PWP_MSGTYPE_BITFIELD = 5,
    PWP_MSGTYPE_REQUEST = 6,
    PWP_MSGTYPE_PIECE = 7,
    PWP_MSGTYPE_CANCEL = 8,
} pwp_msg_type_e;

/**
 * Create a new connection
 * @param if non-null, use this as memory for the connection */
void *pwp_conn_new(void* mem);

void pwp_conn_release(pwp_conn_t* pco);

/**
 * @return peer user data */
void *pwp_conn_get_peer(pwp_conn_t* pco);

void pwp_conn_set_my_peer_id(pwp_conn_t* pco, const char *peer_id);

void pwp_conn_set_their_peer_id(pwp_conn_t* pco, const char *peer_id);

void pwp_conn_set_infohash(pwp_conn_t* pco, const char *infohash);

void pwp_conn_set_peer(pwp_conn_t* pco, void * peer);

int pwp_conn_peer_is_interested(pwp_conn_t* pco);

int pwp_conn_im_choking(pwp_conn_t* pco);

/**
 * @return whether I am choked or not */
int pwp_conn_im_choked(pwp_conn_t* pco);

int pwp_conn_im_interested(pwp_conn_t* pco);

void pwp_conn_unchoke(pwp_conn_t* pco);

int pwp_conn_get_download_rate(const pwp_conn_t* pco);

int pwp_conn_get_upload_rate(const pwp_conn_t* pco);

/**
 * unchoke, choke, interested, uninterested,
 * @return non-zero if unsucessful */
int pwp_conn_send_statechange(pwp_conn_t* pco, const unsigned char msg_type);

/**
 * Send the piece highlighted by this request.
 * @pararm req - the requesting block
 **/
void pwp_conn_send_piece(pwp_conn_t* pco, bt_block_t * req);

/**
 * Tell peer we have this piece 
 * @return 0 on error, 1 otherwise */
int pwp_conn_send_have(pwp_conn_t* pco, const int piece_idx);

/**
 * Send request for a block */
void pwp_conn_send_request(pwp_conn_t* pco, const bt_block_t * request);

/**
 * Tell peer we are cancelling the request for this block */
void pwp_conn_send_cancel(pwp_conn_t* pco, bt_block_t * cancel);

void pwp_conn_set_im_interested(pwp_conn_t* me_);

void pwp_conn_set_piece_info(pwp_conn_t* pco, int num_pieces, int piece_len);

void pwp_conn_set_state(pwp_conn_t* pco, const int state);

int pwp_conn_get_state(pwp_conn_t* pco);

/**
 * Peer told us they have this piece.
 * @return 0 on error, 1 otherwise */
int pwp_conn_mark_peer_has_piece(pwp_conn_t* pco, const int piece_idx);

/**
 * @return number of requests we required from the peer */
int pwp_conn_get_npending_requests(const pwp_conn_t* pco);

/**
 * @return number of requests we will request from the peer */
int pwp_conn_get_npending_peer_requests(const pwp_conn_t* pco);

/**
 * pend a block request */
void pwp_conn_request_block_from_peer(pwp_conn_t* pco, bt_block_t * blk);

void pwp_conn_periodic(pwp_conn_t* pco);

/** 
 *  @return 1 if the peer has this piece; otherwise 0 */
int pwp_conn_peer_has_piece(pwp_conn_t* pco, const int piece_idx);

typedef struct {
    /** send data to peer */
    func_send_f send;

    /* drop the connect.
     * Most likely because we detected an error with the peer's processing */
    func_disconnect_f disconnect;

    /* manage piece related operations */
    func_write_block_to_stream_f write_block_to_stream;

    /**
     * Ask our caller if they have an idea of what block they would like.
     * We're able to request a block from the peer now.
     *
     * @return 0 on success; otherwise -1 on failure*/
    func_pollblock_f pollblock;

    /* We've just downloaded the block and want to allocate it. */
    func_pushblock_f pushblock;

    /* Let caller know that a peer has announced that they have a piece */
    func_peerpiece_f peer_have_piece;

    /* Let caller know that it couldn't download this piece from this peer */
    func_peergiveblockback_f peer_giveback_block;

#if 0
    /**
     * Create lock */
    func_lock_f create_lock;

    /**
     * Obtain mutually exclusive lock. */
    func_lock_f get_lock;

    /**
     * Release mutually exclusive lock. */
    func_lock_f release_lock;
#else
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
#endif

    /* logging */
    func_log_f log;
} pwp_conn_cbs_t;

typedef struct {
    bt_block_t blk;
    const void* data;
} msg_piece_t;

typedef struct {
    uint32_t piece_idx;
} msg_have_t;

typedef struct {
   bitfield_t *bf;
} msg_bitfield_t;

void pwp_conn_choke_peer(pwp_conn_t* pco);

void pwp_conn_unchoke_peer(pwp_conn_t* pco);

void pwp_conn_keepalive(pwp_conn_t* pco);

void pwp_conn_choke(pwp_conn_t* pco);

void pwp_conn_unchoke(pwp_conn_t* pco);

void pwp_conn_interested(pwp_conn_t* pco);

void pwp_conn_uninterested(pwp_conn_t* pco);

void pwp_conn_have(pwp_conn_t* pco, msg_have_t* have);

/**
 * Receive a bitfield */
void pwp_conn_bitfield(pwp_conn_t* pco, msg_bitfield_t* bitfield);

/**
 * Respond to a peer's request for a block
 * @return 0 on error, 1 otherwise */
int pwp_conn_request(pwp_conn_t* pco, bt_block_t *request);

/**
 * Receive a cancel message. */
void pwp_conn_cancel(pwp_conn_t* pco, bt_block_t *cancel);

/**
 * Receive a piece message
 * @return 1 on sucess; otherwise 0 */
int pwp_conn_piece(pwp_conn_t* pco, msg_piece_t *piece);

void pwp_conn_set_cbs(pwp_conn_t* pco, pwp_conn_cbs_t* funcs, void* caller);

/**
 *
 */
int pwp_conn_flag_is_set(pwp_conn_t* pco, const int flag);

/**
 * Tells the peerconn that the connection failed */
void pwp_conn_connect_failed(pwp_conn_t* pco);

/**
 * @return 1 if the request is still pending; otherwise 0 */
int pwp_conn_block_request_is_pending(void* pc, bt_block_t *b);

/**
 * Provide a block for us to request from the peer */
void pwp_conn_offer_block(pwp_conn_t* me_, bt_block_t *b);

// TODO: this could be renamed or documented better
/**
 * Set the progress counter for pieces we've downloaded */
void pwp_conn_set_progress(pwp_conn_t* me_, void* counter);

/**
 * Send a bitfield to peer, telling them what we have
 * @param npieces Number of pieces
 * @param pieces_completed Sparse counter containing pieces we've completed 
 * @param send_cb Callback for sending data
 * @return 1 if successful, 0 otherwise */
int pwp_send_bitfield(
        int npieces,
        void* pieces_completed,
        func_send_f send_cb,
        void* cb_ctx,
        void* peer_udata);

#endif /* PWP_CONNECTION_H */
