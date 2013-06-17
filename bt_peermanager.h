int bt_peermanager_contains(void *pm, const char *ip, const int port);

void *bt_peermanager_netpeerid_to_peer(void * pm, const int netpeerid);

bt_peer_t *bt_peermanager_add_peer(void *pm,
                              const char *peer_id,
                              const int peer_id_len,
                              const char *ip, const int ip_len, const int port);


void bt_peermanager_forall(
        void* pm,
        void* caller,
        void* udata,
        void (*run)(void* caller, void* peer, void* udata));

void* bt_peermanager_new(void* caller);

void bt_peermanager_set_config(void* pm, void* cfg);

int bt_peermanager_count(void* pm);

void* bt_peermanager_get_peer_from_pc(void* pm, const void* pc);
