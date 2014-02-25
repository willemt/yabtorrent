#ifndef BT_STRING_H
#define BT_STRING_H

#if WIN32
int asprintf(char **resultp, const char *format, ...);

char* strndup(const char* str, const unsigned int len);
#endif

char *url2host(const char *url);

char *url2port(const char *url);

#endif /* BT_STRING_H */
