#ifndef TORRENT_READER_H
#define TORRENT_READER_H

void* tfr_new(
    int (*cb_event)(void* udata, const char* key),
    int (*cb_event_str)(void* udata, const char* key, const char* val, int len),
    int (*cb_event_int)(void* udata, const char* key, int val),
    void* udata
    );

void tfr_read_metainfo(
    void *me,
    const char *buf,
    const int len);

#endif /* TORRENT_READER_H */
