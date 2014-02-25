#ifndef BT_PIECE_SELECTOR_H
#define BT_PIECE_SELECTOR_H

typedef void *(
    *func_piece_selector_new_f
)    (
    int size
);

typedef void (
    *func_add_piece_back_f
)    (
    void *self,
    int idx
);

typedef void (
    *func_announce_peer_have_piece_f
)    (
    void *self,
    void *peer,
    int idx
);

typedef int (
    *func_get_best_piece_from_peer_f
)   (
    void *,
    const void *peer
);

#if 0
/*  piece selector interface */
typedef struct
{
    func_piece_selector_new_f new;

    /*  add a piece back to the selector */
    func_add_piece_back_f offer_piece;

    func_mutate_object_with_int_f announce_have_piece;

    func_mutate_object_f remove_peer;

    func_mutate_object_f add_peer;

    func_announce_peer_have_piece_f announce_peer_have_piece;

    func_collection_count_f get_npeers;

    func_collection_count_f get_npieces;

    /* remove best piece */
    func_get_best_piece_from_peer_f poll_best_piece_from_peer;
} bt_pieceselector_i;
#endif

#endif /* BT_PIECE_SELECTOR_H */
