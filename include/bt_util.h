#ifndef BT_UTIL_H
#define BT_UTIL_H

char *url2host(const char *url);

char *bt_generate_peer_id();

#if WIN32
char* strndup(const char* str, const unsigned int len);
#endif

#endif /* BT_UTIL_H */
