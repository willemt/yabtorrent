
bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
);

char *bt_piece_get_hash(bt_piece_t * me);

void *bt_piece_read_block(
    bt_piece_t *pceo,
    void *caller,
    const bt_block_t * blk
);

int bt_piecedb_add_file(
    bt_piecedb_t * db,
    const char *fname,
    const int fname_len,
    const int flen
);

int bt_piece_is_complete(bt_piece_t * me);

int bt_piece_is_downloaded(bt_piece_t * me);

void* bt_piece_get_peers(bt_piece_t *me, int *iter);

int bt_piece_num_peers(bt_piece_t *me);

void bt_piece_drop_download_progress(bt_piece_t *me);

int bt_piece_write_block_to_stream(
    bt_piece_t * me,
    bt_block_t * blk,
    unsigned char ** msg);
