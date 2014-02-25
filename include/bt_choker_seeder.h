
#ifndef BT_CHOKER_SEEDER_PEER_H_
#define BT_CHOKER_SEEDER_PEER_H_

void *bt_seeding_choker_new(const int size);

void bt_seeding_choker_add_peer(void *ckr, void *peer);

void bt_seeding_choker_remove_peer(void *ckr, void *peer);

void bt_seeding_choker_decide_best_npeers(void *ckr);

void bt_seeding_choker_unchoke_peer(void *ckr, void *peer);

void bt_seeding_choker_set_choker_peer_iface(void *ckr,
                                             void *udata,
                                             bt_choker_peer_i * iface);

int bt_seeding_choker_get_npeers(void *ckr);

void bt_seeding_choker_get_iface(bt_choker_i * iface);

#endif /* BT_CHOKER_SEEDER_PEER_H_ */
