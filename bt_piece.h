
bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
);

char *bt_piece_get_hash(bt_piece_t * me);

void *bt_piece_read_block(
    void *pceo,
    void *caller,
    const bt_block_t * blk
);

int bt_piecedb_add_file(
    bt_piecedb_t * db,
    const char *fname,
    const int fname_len,
    const int flen
);
