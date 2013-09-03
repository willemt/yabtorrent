
void *bt_random_selector_new(int npieces);

void bt_random_selector_giveback_piece(
    void *r,
    void* peer,
    int piece_idx);

void bt_random_selector_have_piece(void *r, int piece_idx);

void bt_random_selector_remove_peer(void *r, void *peer);

void bt_random_selector_add_peer(void *r, void *peer);

void bt_random_selector_peer_have_piece(void *r, void *peer, int piece_idx);

int bt_random_selector_get_npeers(void *r);

int bt_random_selector_get_npieces(void *r);

int bt_random_selector_poll_best_piece(void *r, const void *peer);

