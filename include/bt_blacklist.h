
/**
 * @return newly initialised blacklist */
void *bt_blacklist_new();

void bt_blacklist_add_peer(
    void* blacklist,
    void* piece,
    void* peer);

/**
 * Mark the peer as potentially blacklisted for this piece
 * This happens when a peer sends a subset of the blocks of a piece that was
 * invalidated. */
void bt_blacklist_add_peer_as_potentially_blacklisted(
    void* blacklist,
    void* piece,
    void* peer);

/**
 * @return 1 if peer has been blacklisted */
int bt_blacklist_peer_is_blacklisted(
    void* blacklist,
    void* piece,
    void* peer);

/**
 * @return 1 if peer is in the running for becoming blacklisted */
int bt_blacklist_peer_is_potentially_blacklisted(
    void* blacklist,
    void* piece,
    void* peer);

/**
 * @return number pieces within blacklist */
int bt_blacklist_get_npieces(void* blacklist);
