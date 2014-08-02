#ifndef PWP_MSGHANDLER_H
#define PWP_MSGHANDLER_H

typedef struct {
    int (*func)(
        void* mh,
        void *message,
        void* udata,
        const char** buf,
        unsigned int *len);
    void* udata;
} pwp_msghandler_item_t; 

/**
 * @return new msg handler */
void* pwp_msghandler_new2(
        void *pc,
        pwp_msghandler_item_t* custom_handlers,
        int nhandlers,
        unsigned int max_workload_bytes);

/**
 * @return new msg handler */
void* pwp_msghandler_new(void *pc);

/**
 * Release memory used by message handler */
void pwp_msghandler_release(void *mh);

/**
 * Receive this much data.
 * If there is enough data this function will dispatch pwp_connection events
 * @param mh The message handler object
 * @param buf The data to be read in
 * @param len The length of the data to be read in
 * @return 1 if successful, 0 if the peer needs to be disconnected */
int pwp_msghandler_dispatch_from_buffer(void *mh,
        const char* buf,
        unsigned int len);

#endif /* PWP_MSGHANDLER_H */
