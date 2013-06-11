void *bt_diskmem_new();

void bt_diskmem_free( void *dco);

void bt_diskmem_set_size(void *dco, const int piece_bytes_size);

bt_blockrw_i *bt_diskmem_get_blockrw( void *dco);

int bt_diskmem_write_block(
    void *udata,
    void *caller, 
    const bt_block_t * blk,
    const void *blkdata
);

