#ifndef BT_SELECTOR_RARESTFIRST_H
#define BT_SELECTOR_RARESTFIRST_H

void *bt_rarestfirst_selector_new(int npieces);

/**
 * Add this piece back to the selector */
void bt_rarestfirst_selector_giveback_piece(void *r, void* peer, int piece_idx);

void bt_rarestfirst_selector_have_piece(void *r, int piece_idx);

void bt_rarestfirst_selector_remove_peer(void *r, void *peer);

void bt_rarestfirst_selector_add_peer(void *r, void *peer);

/**
 * Let us know that there is a peer who has this piece
 */
void bt_rarestfirst_selector_peer_have_piece(void *r, void *peer,
                                                      int piece_idx);

int bt_rarestfirst_selector_get_npeers(void *r);


int bt_rarestfirst_selector_get_npieces(void *r);

/**
 * Poll best piece from peer,
 * @param r Rarestfirst object
 * @param peer Best piece in context of this peer
 * @return idx of piece which is best; otherwise -1 */
int bt_rarestfirst_selector_poll_best_piece(void *r, const void *peer);

#endif /* BT_SELECTOR_RARESTFIRST_H */
