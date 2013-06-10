void *mocktorrent_new(int size, int piece_len);
void *mocktorrent_get_data(void* _me, unsigned int offset);
void *mocktorrent_get_piece_sha1(void* _me, unsigned int piece);
