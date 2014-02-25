
#ifndef BT_DISKCACHE_H_
#define BT_DISKCACHE_H_

void bt_diskcache_set_func_log(
    void * dco,
    func_log_f log,
    void *udata
);

void *bt_diskcache_new() ;

void bt_diskcache_set_size(
    void *dco,
    const int piece_bytes_size
);

/**
 * Set the blockrw that we want to use to write to disk */
void bt_diskcache_set_disk_blockrw(
    void *dco,
    bt_blockrw_i * irw,
    void *irw_data
);

bt_blockrw_i *bt_diskcache_get_blockrw( void *dco);

/**
 * put all pieces onto disk */
void bt_diskcache_disk_dump( void *dco);

void bt_diskcache_set_piece_length(void* dco, int piece_length);

#endif /* BT_DISKCACHE_H_ */
