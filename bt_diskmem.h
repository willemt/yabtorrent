void *bt_diskmem_new();

void bt_diskmem_free( void *dco);

void bt_diskmem_set_size( void *dco, const int piece_bytes_size);

bt_blockrw_i *bt_diskmem_get_blockrw( void *dco);

