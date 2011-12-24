


typedef struct
{
    int select_timeout_msec;
    int max_peer_connections;
    int max_active_peers;
    int max_cache_mem;
    int tracker_scrape_interval;
} bt_client_cfg_t;

typedef struct
{
    int (
    *tracker_connect
    )   (
    void **udata,
    const char *host,
    const char *port,
    char *my_ip
    );

    int (
    *tracker_send
    )   (
    void **udata,
    const void *send,
    int len
    );

    int (
    *tracker_recv
    )   (
    void **udata,
    char **recv,
    int *rlen
    );

    int (
    *tracker_disconnect
    )   (
    void **udata
    );

    int (
    *peer_connect
    )   (
    void **udata,
    const char *host,
    const char *port,
    int *peerid
    );

    int (
    *peer_send
    )   (
    void **udata,
    const int peerid,
    const unsigned char *send_data,
    const int len
    );

    int (
    *peer_recv_len
    )   (
    void **udata,
    int peerid,
    char *recv,
    int *len
    );

    int (
    *peer_disconnect
    )   (
    void **udata,
    int peerid
    );

    int (
    *peers_poll
    )   (
    void **udata,
    const int msec_timeout,
    int (*func_process) (void *,
                         int),
    void (*func_process_connection) (void *,
                                     int netid,
                                     char *ip,
                                     int),
    void *data
    );

    int (
    *peer_listen_open
    )   (
    void **udata,
    const int port
    );

    int (
    *peer_listen_poll
    )   (
    void **udata,
    int msec_timeout,
    void (*func_process) (void *,
                          int netid,
                          char *ip,
                          int),
    void *data
    );

} bt_net_funcs_t;

/*  bt block */
typedef struct
{
    int piece_idx;
    int block_byte_offset;
    int block_len;
} bt_block_t;

char *bt_generate_peer_id(
);

void bt_set_peer_id(
    void *bto,
    char *peer_id
);

char *bt_client_get_peer_id(
    void *bto
);

void bt_peerconn_send_piece(
    void *pco,
    bt_block_t * request
);

/*----------------------------------------------------------------------------*/
void *bt_client_new(
);

int bt_client_get_num_peers(
    void *bto
);

int bt_client_get_num_pieces(
    void *bto
);

char *bt_client_get_tracker_url(
    void *bto
);

int bt_client_get_total_file_size(
    void *bto
);

char *bt_client_get_info_hash(
    void *bto
);

char *bt_client_get_fail_reason(
    void *bto
);

int bt_client_get_interval(
    void *bto
);

int bt_client_get_nbytes_downloaded(
    void *bto
);

int bt_client_is_failed(
    void *bto
);
