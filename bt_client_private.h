typedef struct
{
    /* the info_hash of the file to be downloaded */
    char* info_hash;

    /* my peerid */
    char* my_peer_id;

    /* database for writing pieces */
    bt_piecedb_i *ipdb;
    void* piecedb;

    /*  tracker client */
    void *tc;

    /* net stuff */
    bt_client_funcs_t func;

    void *net_udata;

    char fail_reason[255];

    /*  are we seeding? */
    int am_seeding;

    /*  logging  */
    func_log_f func_log;
    void *log_udata;

    void* cfg;

    /* peer manager */
    void* pm;

    /*  leeching choker */
    void *lchoke;

    /* timer */
    void *ticker;

} bt_client_t;

