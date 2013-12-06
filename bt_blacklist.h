
void *bt_blacklist_new();

void bt_blacklist_add_peer(
    void* blacklist,
    void* piece,
    void* peer);

void bt_blacklist_add_peer_as_potentially_blacklisted(
    void* blacklist,
    void* piece,
    void* peer);

int bt_blacklist_peer_is_blacklisted(
    void* blacklist,
    void* piece,
    void* peer);

int bt_blacklist_peer_is_potentially_blacklisted(
    void* blacklist,
    void* piece,
    void* peer);

int bt_blacklist_get_npieces(void* blacklist);
