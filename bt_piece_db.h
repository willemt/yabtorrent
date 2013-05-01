/* piece database */
typedef struct
{
    int pass;
} bt_piecedb_t;

bt_piecedb_t *bt_piecedb_new();

void bt_piecedb_set_piece_length(bt_piecedb_t * db, const int piece_length_bytes);

void bt_piecedb_set_tot_file_size(bt_piecedb_t * db, const int tot_file_size_bytes);

int bt_piecedb_get_tot_file_size(bt_piecedb_t * db);

void bt_piecedb_set_diskstorage(bt_piecedb_t * db, bt_blockrw_i * irw, void *udata);

bt_piece_t *bt_piecedb_poll_best_from_bitfield(bt_piecedb_t * db, bitfield_t * bf_possibles);

bt_piece_t *bt_piecedb_get(bt_piecedb_t * db, const int idx);

void bt_piecedb_add(bt_piecedb_t * db, const char *sha1);

int bt_piecedb_get_length(bt_piecedb_t * db);

void bt_piecedb_print_pieces_downloaded(bt_piecedb_t * db);
