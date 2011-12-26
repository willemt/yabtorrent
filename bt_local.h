
typedef unsigned char byte;

#include <stdbool.h>

#define TRUE 1
#define FALSE 0

//#define bool int

#define PC_NONE 0
#define PC_HANDSHAKE_SENT 1<<0
#define PC_HANDSHAKE_RECEIVED 1<<1
#define PC_DISCONNECTED 1<<2
#define PC_BITFIELD_RECEIVED 1<<3
/*  connected to peer */
#define PC_CONNECTED 1<<4
/*  we can't communicate with the peer */
#define PC_UNCONTACTABLE_PEER 1<<5



typedef struct
{
    void *private;
} bt_piececache_t;

/*  peer wire protocol configuration */
typedef struct
{
    int max_pending_requests;
} bt_pwp_cfg_t;

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

/*  bitfield */
typedef struct
{
    uint32_t *bits;
    int size;                   // size in number of bits
} bt_bitfield_t;

/* piece database */
typedef struct
{
    int pass;
//    bt_piece_info_t *pinfo;
} bt_piecedb_t;

/* disk cache */
typedef struct
{
    int pass;
} bt_diskcache_t;

#include "bt_interfaces.h"

/* file dumper */
typedef struct
{
    int piece_size;
    char *path;
    void *files;
    int nfiles;
    bt_blockrw_i irw;
} bt_filedumper_t;

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

// f_f_lywA ==(m€kb€kb (m) ?€ü "pa€kb" :0f=df,xj0
#define bt_pwp_msgtype_to_string(m)\
    PWP_MSGTYPE_CHOKE == (m) ? "CHOKE" :\
    PWP_MSGTYPE_UNCHOKE == (m) ? "UNCHOKE" :\
    PWP_MSGTYPE_INTERESTED == (m) ? "INTERESTED" :\
    PWP_MSGTYPE_UNINTERESTED == (m) ? "UNINTERESTED" :\
    PWP_MSGTYPE_HAVE == (m) ? "HAVE" :\
    PWP_MSGTYPE_BITFIELD == (m) ? "BITFIELD" :\
    PWP_MSGTYPE_REQUEST == (m) ? "REQUEST" :\
    PWP_MSGTYPE_PIECE == (m) ? "PIECE" :\
    PWP_MSGTYPE_CANCEL == (m) ? "CANCEL" : "none"\


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

/*----------------------------------------------------------------------------*/
#define PROTOCOL_NAME "BitTorrent protocol"
#define INFOKEY_LEN 20
#define BLOCK_SIZE 1 << 14      // 16kb
#define PWP_HANDSHAKE_RESERVERD "\0\0\0\0\0\0\0\0"
#define VERSION_NUM 1000
#define PEER_ID_LEN 20
#define INFO_HASH_LEN 20



typedef struct
{
    int length;

    /* 32 characters representing the md5sum of the file */
    char *md5sum;
    char *path;
} bt_file_t;

/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/
bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
);

void bt_piece_set_disk_blockrw(
    bt_piece_t * pce,
    bt_blockrw_i * irw,
    void *udata
);

char *bt_piece_get_hash(
    bt_piece_t * pce
);

void *bt_piece_read_block(
    void *pceo,
    void *caller,
    const bt_block_t * blk
);

bt_piecedb_t *bt_piecedb_new(
);

bt_piece_t *bt_piecedb_get(
    bt_piecedb_t * db,
    const int idx
);

bt_piece_t *bt_piecedb_poll_best_from_bitfield(
    bt_piecedb_t * db,
    bt_bitfield_t * bf_possibles
);

bt_peer_t *bt_peerconn_get_peer(
    void *pco
);

void *bt_peerconn_new(
);

char *url2host(
    const char *url
);

/*----------------------------------------------------------------------------*/
char *read_file(
    const char *name,
    int *len
);

/*----------------------------------------------------------------------------*/

void bt_read_metainfo(
    int id,
    const char *buf,
    int len,
    bt_piece_info_t * pinfo
);

/*----------------------------------------------------------------------------*/

void bt_bitfield_init(
    bt_bitfield_t * bf,
    const int nbits
);

void bt_bitfield_mark(
    bt_bitfield_t * bf,
    const int bit
);

void bt_bitfield_unmark(
    bt_bitfield_t * bf,
    const int bit
);

int bt_bitfield_is_marked(
    bt_bitfield_t * bf,
    const int bit
);

int bt_bitfield_get_length(
    bt_bitfield_t * bf
);


char *bt_bitfield_str(
    bt_bitfield_t * bf
);

/*----------------------------------------------------------------------------*/
void bt_peerconn_set_func_send(
    void *pco,
    func_send_f func
);

void bt_peerconn_set_func_getpiece(
    void *pco,
    func_getpiece_f func
);


void bt_peerconn_set_func_pollblock(
    void *pco,
    func_pollblock_f func
);


void bt_peerconn_set_func_have(
    void *pco,
    func_have_f func
);


void bt_peerconn_set_func_disconnect(
    void *pco,
    func_disconnect_f func
);


void bt_peerconn_set_func_push_block(
    void *pco,
    func_push_block_f func
);


void bt_peerconn_set_func_recv(
    void *pco,
    func_recv_f func
);


void bt_peerconn_set_func_log(
    void *pco,
    func_log_f func
);

char *url2port(
    const char *url
);


void bt_diskcache_set_func_log(
    bt_diskcache_t * dc,
    func_log_f log,
    void *udata
);

void *bt_diskcache_new(
);

void bt_diskcache_set_size(
    void *dco,
    const int piece_bytes_size
);

void bt_diskcache_set_disk_blockrw(
    void *dco,
    bt_blockrw_i * irw,
    void *irw_data
);

bt_blockrw_i *bt_diskcache_get_blockrw(
    void *dco
);

/*----------------------------------------------------------------------------*/

int bt_filedumper_write_block(
    void *flo,
    void *caller,
    const bt_block_t * blk,
    const void *data
);

void *bt_filedumper_read_block(
    void *flo,
    void *caller,
    const bt_block_t * blk
);

const char *bt_filedumper_file_get_path(
    bt_filedumper_t * fl,
    int idx
);


bt_filedumper_t *bt_filedumper_new(
);


void bt_filedumper_add_file(
    bt_filedumper_t * fl,
    const char *fname,
    const int size
);


int bt_filedumper_get_nfiles(
    bt_filedumper_t * fl
);


bt_blockrw_i *bt_filedumper_get_blockrw(
    bt_filedumper_t * fl
);


void bt_filedumper_set_piece_length(
    bt_filedumper_t * fl,
    const int piece_size
);


void bt_filedumper_set_path(
    bt_filedumper_t * fl,
    const char *path
);


void bt_filedumper_set_udata(
    bt_filedumper_t * fl,
    void *udata
);

/*----------------------------------------------------------------------------*/


void *bt_rarestfirst_selector_new(
    int npieces
);

void bt_rarestfirst_selector_offer_piece(
    void *r,
    int piece_idx
);

void bt_rarestfirst_selector_announce_have_piece(
    void *r,
    int piece_idx
);

void bt_rarestfirst_selector_remove_peer(
    void *r,
    void *peer
);

void bt_rarestfirst_selector_add_peer(
    void *r,
    void *peer
);

void bt_rarestfirst_selector_announce_peer_have_piece(
    void *r,
    void *peer,
    int piece_idx
);

int bt_rarestfirst_selector_get_npeers(
    void *r
);


int bt_rarestfirst_selector_get_npieces(
    void *r
);

int bt_rarestfirst_selector_poll_best_piece(
    void *r,
    const void *peer
);

/*----------------------------------------------------------------------------*/

void *bt_leeching_choker_new(
    const int size
);

void bt_leeching_choker_add_peer(
    void *ckr,
    void *peer
);

void bt_leeching_choker_remove_peer(
    void *ckr,
    void *peer
);

void bt_leeching_choker_announce_interested_peer(
    void *cho,
    void *peer
);

void bt_leeching_choker_decide_best_npeers(
    void *ckr
);

void bt_leeching_choker_optimistically_unchoke(
    void *ckr
);

void bt_leeching_choker_unchoke_peer(
    void *ckr,
    void *peer
);

int bt_leeching_choker_get_npeers(
    void *ckr
);

void bt_leeching_choker_set_choker_peer_iface(
    void *ckr,
    void *udata,
    bt_choker_peer_i * iface
);

void bt_leeching_choker_get_iface(
    bt_choker_i * iface
);

/*----------------------------------------------------------------------------*/
void *bt_seeding_choker_new(
    const int size
);

void bt_seeding_choker_add_peer(
    void *ckr,
    void *peer
);

void bt_seeding_choker_remove_peer(
    void *ckr,
    void *peer
);

void bt_seeding_choker_decide_best_npeers(
    void *ckr
);

void bt_seeding_choker_unchoke_peer(
    void *ckr,
    void *peer
);

void bt_seeding_choker_set_choker_peer_iface(
    void *ckr,
    void *udata,
    bt_choker_peer_i * iface
);

int bt_seeding_choker_get_npeers(
    void *ckr
);

void bt_seeding_choker_get_iface(
    bt_choker_i * iface
);

/*----------------------------------------------------------------------------*/

void *bt_ticker_new(
);

void bt_ticker_push_event(
    void *ti,
    int nsecs,
    void *udata,
    void (*func) (void *)
);

void bt_ticker_step(
    void *ti
);

/*----------------------------------------------------------------------------*/
char *str2sha1hash(
    const char *str,
    int len
);

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
void *bt_diskmem_new(
);

bt_blockrw_i *bt_diskmem_get_blockrw(
    void *dco
);
