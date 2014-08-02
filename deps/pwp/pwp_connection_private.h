#ifndef PWP_CONNECTION_PRIVATE_H
#define PWP_CONNECTION_PRIVATE_H

/*  state */
typedef struct
{
    /* TODO: this should be removed */
    /* this bitfield indicates which pieces the peer has */
    //bitfield_t have_bitfield;

    /* for recording state machine's state */
    unsigned int flags;

    /* count number of failed connections */
    int failed_connections;

    /* current tick */
    int tick;

} peer_connection_state_t;

typedef struct
{
    /* the tick which this request was made */
    int tick;
    bt_block_t blk;
} request_t;

/*  peer connection */
typedef struct
{
    peer_connection_state_t state;

    unsigned int bytes_downloaded_this_period,
                 bytes_uploaded_this_period;

    /* Download/upload rate measurement */
    void *bytes_drate,
        *bytes_urate;

    /* Pending requests that we are waiting to get
     * We could receive pieces that are a subset of the original request */
    hashmap_t *recv_reqs;

    /* Pending requests we are fufilling for the peer */
    linked_list_queue_t *peer_reqs;
    
    /* list of requests to make */
    linked_list_queue_t *reqs;
    void *req_lock;

    // TODO: need to remove this
    /* need the piece_length to check pieces sent/rcvd are well formed */
    int piece_len;

    // TODO: need to remove this
    int num_pieces;

    /* info of who we are connected to */
    void *peer_udata;

    /* callbacks */
    pwp_conn_cbs_t cb;
    void *cb_ctx;

    /* we obtain this read only counter from our caller (ie. cb_ctx) */
    const chunkybar_t *pieces_completed;

    /* pieces that the piece has */
    chunkybar_t *pieces_peerhas;

} pwp_conn_private_t;

#endif /* PWP_CONNECTION_PRIVATE_H */
