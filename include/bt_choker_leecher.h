
#ifndef BT_CHOKER_LEECHER_H_
#define BT_CHOKER_LEECHER_H_

void *bt_leeching_choker_new(const int size);

void bt_leeching_choker_add_peer(void *ckr, void *peer);

void bt_leeching_choker_remove_peer(void *ckr, void *peer);

void bt_leeching_choker_announce_interested_peer(void *cho, void *peer);

void bt_leeching_choker_decide_best_npeers(void *ckr);

void bt_leeching_choker_optimistically_unchoke(void *ckr);

void bt_leeching_choker_unchoke_peer(void *ckr, void *peer);

int bt_leeching_choker_get_npeers(void *ckr);

void bt_leeching_choker_set_choker_peer_iface(void *ckr,
                                              void *udata,
                                              bt_choker_peer_i * iface);

void bt_leeching_choker_get_iface(bt_choker_i * iface);

#endif /* BT_CHOKER_LEECHER_H_ */
