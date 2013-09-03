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

    func_mutate_object_f remove_peer;

    func_mutate_object_f add_peer;

    func_mutate_object_f unchoke_peer;

    func_collection_count_f get_npeers;

    func_choker_set_peer_iface set_choker_peer_iface;

    func_mutate_f decide_best_npeers;
} bt_choker_i;

