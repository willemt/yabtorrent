#ifndef TRACKER_CLIENT_PRIVATE_H
#define TRACKER_CLIENT_PRIVATE_H

typedef struct
{
    /* the length of a piece (from protocol) */
    int piece_len;
    /* number of pieces (from protocol) */
    int npieces;
} bt_piece_info_t;

typedef struct
{
    /* how often we must send messages to the tracker */
    int interval;

    const char* uri;

    /* so that we remember when we last requested the peer list */
    time_t last_tracker_request;

    void (*on_work_done)(void* callee, int status);

    void (*on_add_peer)(void* callee,
        char* peer_id,
        unsigned int peer_id_len,
        char* ip,
        unsigned int ip_len,
        unsigned int port);

    void* callee;

    void *cfg;

    /** callback for initiating read of metafile */
    void (*func_read_metafile) (void *, char *, int len);
    void *udata;

} trackerclient_t;

int bt_trackerclient_read_tracker_response(
    trackerclient_t* me,
    char *buf,
    int len);

/**
 * Receive this much data on this step.
 * @param me_ Tracker client
 * @param buf Buffer to dispatch events from
 * @param len Length of buffer */
void thttp_dispatch_from_buffer(
        void *me_,
        const unsigned char* buf,
        unsigned int len);

/**
 * Connect to URL
 * @return 1 on success; 0 otherwise */
int thttp_connect(void *me_, const char* url);


void thttp_connected(void *me_);

int net_tcp_connect(const char *host, const char *port);

int trackerclient_read_tracker_response(
    trackerclient_t* me,
    const char *buf,
    int len);

#endif /* TRACKER_CLIENT_PRIVATE_H */
