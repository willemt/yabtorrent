
int bt_filedumper_write_block(
    void *flo,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk,
    const void *data);


void *bt_filedumper_read_block(
    void *flo,
    void *caller __attribute__((__unused__)),
    const bt_block_t * blk
);

void *bt_filedumper_new();

void bt_filedumper_add_file(
    void* fl,
    const char *fname,
    int fname_len,
    const int size);

int bt_filedumper_get_nfiles( void * fl);

const char *bt_filedumper_file_get_path(
    void * fl,
    const int idx
);

bt_blockrw_i *bt_filedumper_get_blockrw( void * fl);

void bt_filedumper_set_piece_length( void * fl, const int piece_size);

void bt_filedumper_set_cwd( void * fl, const char *path);

unsigned int bt_filedumper_get_total_size(void * fl);
