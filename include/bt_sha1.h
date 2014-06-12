#ifndef BT_SHA1_H
#define BT_SHA1_H

int bt_sha1_equal(char * s1, char * s2);

void bt_str2sha1hash(
    unsigned char *hash_out,
    const unsigned char *str,
    int len);

#endif /* BT_SHA1_H */
