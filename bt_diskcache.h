void bt_diskcache_set_func_log(
    bt_diskcache_t * dc,
    func_log_f log,
    void *udata
);

void *bt_diskcache_new() ;

void bt_diskcache_set_size(
    void *dco,
    const int piece_bytes_size
);

void bt_diskcache_set_disk_blockrw(
    void *dco,
    bt_blockrw_i * irw,
    void *irw_data
);

bt_blockrw_i *bt_diskcache_get_blockrw( void *dco);

void bt_diskcache_disk_dump( void *dco);
