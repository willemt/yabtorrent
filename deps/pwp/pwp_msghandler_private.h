#ifndef PWP_MSGHANDLER_PRIVATE_H
#define PWP_MSGHANDLER_PRIVATE_H

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

typedef struct {
    uint32_t len;
    char id;
    unsigned int bytes_read;
    unsigned int tok_bytes_read;
    union {
        msg_have_t hve;
        msg_bitfield_t bf;
        bt_block_t blk;
        msg_piece_t pce;
    };
} msg_t;

typedef struct msghandler_item_s msghandler_item_t;
typedef struct pwp_msghandler_private_s pwp_msghandler_private_t; 

struct pwp_msghandler_private_s {
    /* current message we are reading */
    msg_t msg;

    /* peer connection */
    void* pc;

    void* udata;

    int (*process_item)(
        pwp_msghandler_private_t* me,
        msg_t *m,
        void* udata,
        const char** buf,
        unsigned int *len);

    int nhandlers;

    msghandler_item_t* handlers;
};

struct msghandler_item_s {
    int (*func)(
        pwp_msghandler_private_t* me,
        msg_t *m,
        void* udata,
        const char** buf,
        unsigned int *len);
    void* udata;
}; 

void mh_endmsg(pwp_msghandler_private_t* me);

int mh_uint32(
        uint32_t* in,
        msg_t *msg,
        const char** buf,
        unsigned int *len);

int mh_byte(
        char* in,
        unsigned int *tot_bytes_read,
        const char** buf,
        unsigned int *len);

#endif /* PWP_MSGHANDLER_PRIVATE_H */
