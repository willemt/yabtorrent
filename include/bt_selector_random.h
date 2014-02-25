#ifndef BT_SELECTOR_RANDOM_H
#define BT_SELECTOR_RANDOM_H

void *bt_random_selector_new(int npieces);

/**
 * Add this piece back to the selector.
 * This is usually when we want to make the piece a candidate again
 *
 * @param peer The peer that is giving it back.
 * @param piece_idx The piece
 */
void bt_random_selector_giveback_piece(
    void *r,
    void* peer,
    int piece_idx);

/**
 * Notify selector that we have this piece */
void bt_random_selector_have_piece(void *r, int piece_idx);

void bt_random_selector_remove_peer(void *r, void *peer);

void bt_random_selector_add_peer(void *r, void *peer);

/**
 * Let us know that there is a peer who has this piece
 */
void bt_random_selector_peer_have_piece(void *r, void *peer, int piece_idx);

int bt_random_selector_get_npeers(void *r);

int bt_random_selector_get_npieces(void *r);

/**
 * Poll best piece from peer
 * @param r random object
 * @param peer Best piece in context of this peer
 * @return idx of piece which is best; otherwise -1 */
int bt_random_selector_poll_best_piece(void *r, const void *peer);

#endif /* BT_SELECTOR_RANDOM_H */
