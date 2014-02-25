
#ifndef BT_CHOKER_H_
#define BT_CHOKER_H_

/**
 * Create a new leeching choker
 * @param max_unchoked_connections maximum number of unchoked peers
 */
typedef void *(
    *func_new_choker_f
)    (
    const int max_unchoked_connections
);

typedef void (
    *func_mutate_object_f
)    (
    void *,
    void *
);

typedef void (
    *func_mutate_object_with_int_f
)    (
    void *,
    int
);

typedef int (
    *func_collection_count_f
)   (
    void *
);

typedef void (
    *func_mutate_f
)    (
    void *
);


typedef void (
    *func_choker_set_peer_iface
)    (
    void *ckr,
    void *udata,
    bt_choker_peer_i * iface
);

/*  chocker */
typedef struct
{
    func_new_choker_f new;

    /**
     * Stop managing this peer */
    func_mutate_object_f remove_peer;

    /**
     * Start managing a new peer */
    func_mutate_object_f add_peer;

    func_mutate_object_f unchoke_peer;

    /**
     * Get number of peers */
    func_collection_count_f get_npeers;

    /**
     * Get choker interface
     */
    func_choker_set_peer_iface set_choker_peer_iface;

    func_mutate_f decide_best_npeers;
} bt_choker_i;

#endif /* BT_CHOKER_H_ */
