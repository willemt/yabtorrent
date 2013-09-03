void *bt_rarestfirst_selector_new(int npieces);

void bt_rarestfirst_selector_giveback_piece(void *r, void* peer, int piece_idx);

void bt_rarestfirst_selector_have_piece(void *r, int piece_idx);

void bt_rarestfirst_selector_remove_peer(void *r, void *peer);

void bt_rarestfirst_selector_add_peer(void *r, void *peer);

void bt_rarestfirst_selector_peer_have_piece(void *r,
                                                      void *peer,
                                                      int piece_idx);

int bt_rarestfirst_selector_get_npeers(void *r);


int bt_rarestfirst_selector_get_npieces(void *r);

int bt_rarestfirst_selector_poll_best_piece(void *r, const void *peer);


