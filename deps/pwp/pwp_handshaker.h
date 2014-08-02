#ifndef PWP_HANDSHAKER_H
#define PWP_HANDSHAKER_H

typedef struct {
    /* protocol name */
    int pn_len;
    char* pn;
    char* reserved;
    char* infohash;
    char* peerid;
} pwp_handshake_t;

/**
 * Create a new handshaker
 * @return newly initialised handshaker */
void* pwp_handshaker_new(char* expected_info_hash, char* mypeerid);

/**
 * Release memory used by handshaker */
void pwp_handshaker_release(void* hs);

/**
 *  Receive handshake from other end
 *  Disconnect on any errors
 *  @return 1 on succesful handshake; 0 on unfinished reading; -1 on failed handshake */
int pwp_handshaker_dispatch_from_buffer(void* me_, const char** buf, unsigned int* len);

/**
 * Send the handshake
 * @return 0 on failure; 1 otherwise */
int pwp_send_handshake(
        void* callee,
        void* udata,
        int (*send)(void *callee, const void *udata, const void *send_data, const int len),
        char* expected_ih,
        char* my_pi);

/**
 * @return null if handshake was successful */
pwp_handshake_t* pwp_handshaker_get_handshake(void* me_);

#endif /* PWP_HANDSHAKER_H */
