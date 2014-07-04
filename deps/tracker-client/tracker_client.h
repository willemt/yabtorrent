#ifndef TRACKER_CLIENT_H
#define TRACKER_CLIENT_H

/**
 * Initiliase the tracker client
 * @return tracker client on sucess; otherwise NULL */
void *trackerclient_new(
    void (*on_work_done)(void* callee, int status),
    void (*on_add_peer)(void* callee,
        char* peer_id,
        unsigned int peer_id_len,
        char* ip,
        unsigned int ip_len,
        unsigned int port),
    void* callee);

/**
 * Release all memory used by the tracker client
 * @return 1 if successful; 0 otherwise */
int trackerclient_release(void *bto);

int trackerclient_set_opt_int(void *bto, const char *key, const int val);

/**
 * Tell if the uri is supported or not.
 * @param uri URI to check if we support
 * @return 1 if the uri is supported, 0 otherwise */
int trackerclient_supports_uri(void* _me, const char* uri);

/**
 * Connect to the URI.
 * @param uri URI to connnect to
 * @return 1 if successful, 0 otherwise */
int trackerclient_connect_to_uri(void* _me, const char* uri);

/**
 * Set configuration of tracker client
 * @param me_ Tracker client
 * @param cfg Configuration to reference/use */
void trackerclient_set_cfg(void *me_, void *cfg);

/**
 * Receive this much data on this step.
 * @param me_ Tracker client
 * @param buf Buffer to dispatch events from
 * @param len Length of buffer */
void trackerclient_dispatch_from_buffer(
        void *me_,
        const unsigned char* buf,
        unsigned int len);


#endif /* TRACKER_CLIENT_H */
