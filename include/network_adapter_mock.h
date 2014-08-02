#ifndef NETWORK_ADAPTER_MOCK_H
#define NETWORK_ADAPTER_MOCK_H

/* connect status */
typedef enum {
    /* not connected from our side, we need to accept the connection */
    CS_NONE,
    CS_REQUESTED,
    /*  connected */
    CS_CONNECTED
} connect_status_e;

/**
 * Each client has many connections.
 * Each connection has an inbox */
typedef struct {
    void* inbox;

    /* id that we use to identify the peer */
    void* nethandle;

    connect_status_e connect_status;
} client_connection_t;

/* 
 *
 */
typedef struct {
    /* there is a connection for each peer */
    void* connections;

    /* the bitorrent client that represents this client */
    void* bt;

    /* id that we use to identify ourselves client.
     * This proxies our IP address */
    void *nethandle;
} client_t;

extern void *__clients;

client_t* networkfuncs_mock_get_client_from_id(void* nethandle);

void* networkfuns_mock_client_new(void* nethandle);

int network_poll(void* caller, void **udata,
               const int msec_timeout,
               int (*func_process) (void *caller,
                                    void* nethandle,
                                    const char* buf,
                                    unsigned int len),
               void (*func_process_connection) (void *,
                                                void* nethandle,
                                                char *ip,
                                                int port));

#endif /* NETWORK_ADAPTER_MOCK_H */
