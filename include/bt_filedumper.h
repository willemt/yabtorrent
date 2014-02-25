
#ifndef BT_FILEDUMPER_H_
#define BT_FILEDUMPER_H_

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

/**
 * Add this file to the bittorrent client
 * This is used for adding new files.
 *
 * @param fname file name
 * @param fname_len length of fname
 * @param flen length in bytes of the file
 * @return 1 on sucess; otherwise 0 */
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

/**
 * Piece_length is required for figuring out where we are writing blocks */
void bt_filedumper_set_piece_length( void * fl, const int piece_size);

void bt_filedumper_set_cwd( void * fl, const char *path);

/**
 * @return total file size in bytes */
unsigned int bt_filedumper_get_total_size(void * fl);

#endif /* BT_FILEDUMPER_H_ */
