

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

/** cfg */
typedef struct
{
    int select_timeout_msec;
    int max_peer_connections;
    int max_active_peers;
    int max_cache_mem;
    int tracker_scrape_interval;
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

    char *tracker_url;
} bt_client_cfg_t;

typedef struct
{
    int (*peer_connect) (void **udata,
                         const char *host, const char *port, int *peerid);

    int (*peer_send) (void **udata,
                      const int peerid,
                      const unsigned char *send_data, const int len);

    int (*peer_recv_len) (void **udata, int peerid, char *recv, int *len);

    int (*peer_disconnect) (void **udata, int peerid);

    int (*peers_poll) (void **udata,
                       const int msec_timeout,
                       int (*func_process) (void *,
                                            int),
                       void (*func_process_connection) (void *,
                                                        int netid,
                                                        char *ip,
                                                        int), void *data);

    int (*peer_listen_open) (void **udata, const int port);

    int (*peer_listen_poll) (void **udata,
                             int msec_timeout,
                             void (*func_process) (void *,
                                                   int netid,
                                                   char *ip, int), void *data);

} bt_net_pwp_funcs_t;

typedef struct
{
    int (*tracker_connect) (void **udata,
                            const char *host, const char *port, char *my_ip);

    int (*tracker_send) (void **udata, const void *send, int len);

    int (*tracker_recv) (void **udata, char **recv, int *rlen);

    int (*tracker_disconnect) (void **udata);
} bt_net_tracker_funcs_t;


/*  bt block */
typedef struct
{
    int piece_idx;
    int block_byte_offset;
    int block_len;
} bt_block_t;

/*  bittorrent piece */
typedef struct
{
    /* index on 'bit stream' */
    const int idx;

} bt_piece_t;

/* peer */
typedef struct
{
    /* 20 byte sha1 string */
    char *peer_id;
    char *ip;
    char *port;

    /* for network api */
    int net_peerid;
} bt_peer_t;

/*----------------------------------------------------------------------------*/

char *bt_generate_peer_id();

void bt_set_peer_id(void *bto, char *peer_id);

char *bt_client_get_peer_id(void *bto);

void bt_peerconn_send_piece(void *pco, bt_block_t * request);

/*----------------------------------------------------------------------------*/
void *bt_client_new();

bt_piece_t *bt_client_get_piece(void *bto, const int piece_idx);

int bt_client_get_num_peers(void *bto);

int bt_client_get_num_pieces(void *bto);

char *bt_client_get_tracker_url(void *bto);

int bt_client_get_total_file_size(void *bto);

char *bt_client_get_fail_reason(void *bto);

int bt_client_get_nbytes_downloaded(void *bto);

int bt_client_is_failed(void *bto);

bt_peer_t *bt_client_add_peer(void *bto,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port);

char *bt_client_get_opt_string(void *bto, const char *key);

void bt_client_set_pwp_net_funcs(void *bto, bt_net_pwp_funcs_t * net);

/*----------------------------------------------------------------------------*/

void *bt_tracker_client_new(bt_client_cfg_t * cfg,
                            void (*metafile) (void *, char *, int len),
                            void *udata);
