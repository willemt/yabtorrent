

#define TRUE 1
#define FALSE 0

typedef struct
{

    /* 20 byte sha1 string */
    char *peer_id;
    char *ip;
    char *port;

} bt_peer_t;

//    Choked:
//      When true, this flag means that the choked peer is not allowed to
//      request data. 
#define PEER_STATEF_CHOKED (1<<1)

    // Interested:
    //    When true, this flag means a peer is interested in requesting data
    //    from another peer. This indicates that the peer will start requesting
    //    blocks if it is unchoked. 
#define PEER_STATEF_INTERESTED (1<<2)

typedef enum
{
    PWP_MSGTYPE_CHOKE = 0,
    PWP_MSGTYPE_UNCHOKE = 1,
    PWP_MSGTYPE_INTERESTED = 2,
    PWP_MSGTYPE_UNINTERESTED = 3,
    PWP_MSGTYPE_HAVE = 4,
    PWP_MSGTYPE_BITFIELD = 5,
    PWP_MSGTYPE_REQUEST = 6,
    PWP_MSGTYPE_PIECE = 7,
    PWP_MSGTYPE_CANCEL = 8,
} pwp_msg_type_e;


void stream_write_ubyte(
    unsigned char **bytes,
    unsigned char value
);

void stream_write_uint32(
    unsigned char **bytes,
    uint32_t value
);

unsigned char stream_read_ubyte(
    unsigned char **bytes
);

uint32_t stream_read_uint32(
    unsigned char **bytes
);
