#ifndef BT_PIECE_H
#define BT_PIECE_H

/**
 * @return newly initialised piece */
bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
);

/**
 * @param sha1sum The 20 byte hash that describes the content of this piece */
void bt_piece_set_hash(bt_piece_t * me, const unsigned char *sha1sum);

void bt_piece_set_size(bt_piece_t * me, const unsigned int piece_bytes_size);

/**
 * @param hash is expected to be 20 chars long
 * @return 0 on error */
int bt_piece_calculate_hash(bt_piece_t* me, char *hash);

/**
 * @return hash of piece */
char *bt_piece_get_hash(bt_piece_t * me);

/**
 * Determine if the piece is complete
 * A piece needs to be valid to be complete
 *
 * @return 1 if complete; 0 otherwise */
int bt_piece_is_complete(bt_piece_t * me);

/**
 * @return 1 if piece completely downloaded, otherwise 0 */
int bt_piece_is_downloaded(bt_piece_t * me);

/**
 * Get peers based off iterator
 * @param iter Iterator that we use to obtain the next peer. Starts at 0
 * @return next peer */
void* bt_piece_get_peers(bt_piece_t *me, int *iter);

/**
 * @return number of peers that were involved in downloading this piece */
int bt_piece_num_peers(bt_piece_t *me);

/**
 * Have we confirmed the validity of the download using sha1?
 * @return 1 if valid; 0 if invalid; -1 if unchecked */
int bt_piece_is_valid(bt_piece_t * me);

void bt_piece_drop_download_progress(bt_piece_t *me);

/**
 * Build the following request block for the peer, from this piece.
 * Assume that we want to complete the piece by going through the piece in 
 * sequential blocks. */
void bt_piece_poll_block_request(bt_piece_t * me, bt_block_t * request);

void bt_piece_giveback_block(bt_piece_t * me, bt_block_t * b);

void bt_piece_set_complete(bt_piece_t * me, int yes);

void bt_piece_set_idx(bt_piece_t * me, const int idx);

int bt_piece_get_idx(bt_piece_t * me);

/**
 * I/O performed.
 *
 * @return data of piece */
void *bt_piece_get_data(bt_piece_t * me);

#define BT_PIECE_VALIDATE_COMPLETE_PIECE 1
#define BT_PIECE_VALIDATE_ERROR 0
#define BT_PIECE_VALIDATE_INVALID_PIECE -1

/**
 * Validate the piece
 * I/O performed.
 *
 * @return 1 if valid, -1 if invalid, otherwise 0 */
int bt_piece_validate(bt_piece_t* me);

/**
 * I/O performed.
 * @return size of piece */
int bt_piece_get_size(bt_piece_t * me);

/**
 * I/O performed.
 * @return data that the block represents */
void *bt_piece_read_block(bt_piece_t *pceo, void *caller, const bt_block_t * b);

#define BT_PIECE_WRITE_BLOCK_COMPLETELY_DOWNLOADED 2
#define BT_PIECE_WRITE_BLOCK_SUCCESS 1

/**
 * Add this data to the piece
 * I/O performed.
 * @return 1 on success, 2 if now completely downloaded, otherwise 0 */
int bt_piece_write_block(
    bt_piece_t *me,
    void *caller,
    const bt_block_t * b,
    const void *b_data,
    void* peer);

/**
 * Write the block to the byte stream
 * I/O performed.
 * @return 1 on success, 0 otherwise */
int bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    unsigned char ** msg);

/**
 * I/O performed.
 * @return 1 on success, 0 otherwise */
int bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    unsigned char ** msg);

#endif /* BT_PIECE_H */
