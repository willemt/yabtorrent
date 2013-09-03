
/*  commands that chokers use for querying/modifiying peers */
typedef struct
{
    int (*get_drate)(const void*, const void* peer);

    int (*get_urate)(const void*, const void* peer);

    int (*get_is_interested)(void*, void* peer);

    void (*choke_peer)(void*,void*);

    void (*unchoke_peer)(void*,void*);

} bt_choker_peer_i;

