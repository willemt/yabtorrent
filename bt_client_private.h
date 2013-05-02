typedef struct
{
    /* name of the topmost directory */
    char *name;

    /* the main directory of the file */
    char *path;

//    void **peerconnects;
//    int npeers;

    /*  the piece container */
    bt_piecedb_t *db;

    /*  disk cache */
    void *dc;

    /*  tracker client */
    void *tc;

    /*  file dumper */
    void *fd;

    /* net stuff */
    bt_net_pwp_funcs_t net;

    void *net_udata;

    /* number of complete peers */
//    int ncomplete_peers;

    char fail_reason[255];

    /*  are we seeding? */
    int am_seeding;

    /*  logging  */
    func_log_f func_log;
    void *log_udata;

//    bt_client_cfg_t cfg;
//    bt_pwp_cfg_t pwpcfg;

    void* cfg;

    /*  leeching choker */
    void *lchoke;

    /* timer */
    void *ticker;

} bt_client_t;
