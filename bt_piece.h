
bt_piece_t *bt_piece_new(
    const unsigned char *sha1sum,
    const int piece_bytes_size
);

void bt_piecedb_add_file(
    bt_piecedb_t * db,
    const char *fname,
    const int size);
