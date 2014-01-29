
/**
 * @return newly initialised piece */
bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
);

/**
 * @return hash of piece */
char *bt_piece_get_hash(bt_piece_t * me);

/**
 * @return size of piece */
int bt_piece_get_size(bt_piece_t * me);

/**
 * @return data that the block represents */
void *bt_piece_read_block(bt_piece_t *pceo, void *caller, const bt_block_t * b);

/**
 * Determine if the piece is complete
 * A piece needs to be valid to be complete
 *
 * @return 1 if complete; 0 otherwise */
int bt_piece_is_complete(bt_piece_t * me);

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
 * Add this data to the piece
 * @return 1 on success, 2 if now complete, -1 if now invalid, otherwise 0 */
int bt_piece_write_block(
    bt_piece_t *me,
    void *caller,
    const bt_block_t * b,
    const void *b_data,
    void* peer);

/**
 * Read data and use sha1 to determine if valid
 * @return 1 if valid; 0 otherwise */
int bt_piece_is_valid(bt_piece_t * me);

void bt_piece_drop_download_progress(bt_piece_t *me);

/**
 * Write the block to the byte stream
 * @return 1 on success, 0 otherwise */
int bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    unsigned char ** msg);

int bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    unsigned char ** msg);

/**
 * Build the following request block for the peer, from this piece.
 * Assume that we want to complete the piece by going through the piece in 
 * sequential blocks. */
void bt_piece_poll_block_request(bt_piece_t * me, bt_block_t * request);

void bt_piece_giveback_block(bt_piece_t * me, bt_block_t * b);

void bt_piece_set_complete(bt_piece_t * me, int yes);

void bt_piece_set_idx(bt_piece_t * me, const int idx);

int bt_piece_get_idx(bt_piece_t * me);
