
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
    int peerid;

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
    int peerid;
} client_t;

extern void *__clients;

client_t* networkfuncs_mock_get_client_from_id(int peerid);

void* networkfuns_mock_client_new(int id);

