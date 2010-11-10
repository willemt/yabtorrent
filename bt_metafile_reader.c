#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>

#include "sha1.h"

#include "bencode.h"

char *str2sha1hash(
    const char *str,
    int len
);

static void __do_files_list(
    int id,
    bencode_t * list
)
{
    while (bencode_list_has_next(list))
    {
        bencode_t dict;

        long int current_file_len = 0;

        bencode_list_get_next(list, &dict);

        while (bencode_dict_has_next(&dict))
        {
            bencode_t benk;

            const char *key;

            int klen;

            bencode_dict_get_next(&dict, &benk, &key, &klen);

            /*
             * 'length':
             * This is an integer indicating the total length of the file in
             * bytes.
             */
            if (!strncmp(key, "length", klen))
            {
                bencode_int_value(&benk, &current_file_len);
            }
            /*
             * 'md5sum':
             * This is an OPTIONAL value.if included
             * it must be a string of 32 characters corresponding to the MD5
             * sum of the file.This value is not used in BTP / 1.0.
             */
            if (!strncmp(key, "md5sum", klen))
            {

            }
            /*
             * 'path':
             * This is a list of string elements that specify the path of the
             * file, relative to the topmost directory.The last element in the
             * list is the name of the file, and the elements preceding it
             * indicate the directory hierarchy in which this file is situated.
             */
            else if (!strncmp(key, "path", klen))
            {
                char *fullpath;

                int fp_len;     // count including last nul byte

                fullpath = NULL;

                while (bencode_list_has_next(&benk))
                {
                    const char *path;

                    bencode_t pathitem;

                    bencode_list_get_next(&benk, &pathitem);
                    bencode_string_value(&pathitem, &path, &klen);

                    if (!fullpath)
                    {
                        fullpath = strndup(path, klen);
                        fp_len = klen + 1;
                    }
                    else
                    {
                        fullpath = realloc(fullpath, fp_len + klen + 2);
                        fullpath[fp_len - 1] = '/';
                        strncpy(fullpath + fp_len, path, klen);
                        fp_len += klen + 2;
                    }
                }
                bt_add_file(id, fullpath, fp_len, current_file_len);
            }
        }
    }
}

static void __do_info_dict(
    int id,
    bencode_t * dict
)
{
#if 0
    bool got_length = FALSE;

    char *name = NULL;

    int name_len;
#endif

    {
        const char *val;

        int len;

        bencode_dict_get_start_and_len(dict, &val, &len);
//        printf("hash: %s\n", ));

        bt_set_info_hash(id, url_encode(str2sha1hash(val, len)));
        //bt_set_info_hash(id, str2sha1hash(val, len));
    }

    while (bencode_dict_has_next(dict))
    {
        const char *key;

        int klen;

        bencode_t benk;

        bencode_dict_get_next(dict, &benk, &key, &klen);
//        printf("ZZZZEKY %.*s\n", klen, key);
        /* is a single file torrent */
        if (!strncmp(key, "length", klen))
        {
            long int file_len;

            bencode_int_value(&benk, &file_len);
//            printf("got filelen %d\n", file_len);
            bencode_dict_get_next(dict, &benk, &key, &klen);
            if (!strncmp(key, "name", klen))
            {
                int len;

                const char *name;

                bencode_string_value(&benk, &name, &len);
                bt_add_file(id, name, len, file_len);
            }
        }
        else if (!strncmp(key, "files", klen))
        {
            __do_files_list(id, &benk);
        }
        else if (!strncmp(key, "piece length", klen))
        {
            int len;

            long int piece_len;

            bencode_int_value(&benk, &piece_len);
            bt_set_piece_length(id, piece_len, len);
        }
        else if (!strncmp(key, "pieces", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            bt_add_pieces(id, val, len);
//            printf("%.*s\n", len, val);
        }
    }
}

char *str2sha1hash(
    const char *str,
    int len
)
{
    SHA1Context sha;

    SHA1Reset(&sha);
    SHA1Input(&sha, str, len);
    if (!SHA1Result(&sha))
    {
        fprintf(stderr, "sha: could not compute message digest for %s\n", str);
        return NULL;
    }
    else
    {
        char *hash;

        char *dst, *src;

        int ii;

#if 1
        hash = malloc(sizeof(char) * 20);
        dst = hash;
        src = sha.Message_Digest;

        for (ii = 0; ii < 5; ii++)
            sha.Message_Digest[ii] = htonl(sha.Message_Digest[ii]);
//        for (ii = 0; ii < 20; ii++, dst++, src++)
//            *dst = *src;
        memcpy(hash, sha.Message_Digest, sizeof(char) * 20);
#else
        asprintf(&hash,
                 "%08x%08x%08x%08x%08x",
                 sha.Message_Digest[0],
                 sha.Message_Digest[1],
                 sha.Message_Digest[2],
                 sha.Message_Digest[3], sha.Message_Digest[4]);
#endif
//        printf("hash: %s\n", hash);
        return hash;
    }
}

/**
 *
 * This assigns the info_hash
 * */
void bt_read_metainfo(
    const int id,
    const char *buf,
    const int len
//    const char *fname
)
{
//    bencode_next(&ben);
//    if (bencode_is_dict(&ben))
    bencode_t ben;

    bencode_init(&ben, buf, len);
//    printf("bencode->len = %d\n", len);

    if (!bencode_is_dict(&ben))
    {
        return;
    }

    while (bencode_dict_has_next(&ben))
    {
        int klen;

        const char *key;

        bencode_t benk;

        bencode_dict_get_next(&ben, &benk, &key, &klen);
        printf("zey: %.*s\n", klen, key);
        if (!strncmp(key, "announce", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            bt_set_tracker_url(id, val, len);
        }
        else if (!strncmp(key, "announce-list", klen))
        {

            while (bencode_list_has_next(&benk))
            {
                bencode_t innerlist;

                bencode_list_get_next(&benk, &innerlist);
                while (bencode_list_has_next(&innerlist))
                {
                    bencode_t benlitem;

                    bencode_list_get_next(&innerlist, &benlitem);
                    const char *backup;

                    int len;

                    bencode_string_value(&benlitem, &backup, &len);
                    bt_add_tracker_backup(id, backup, len);
                }
            }
        }
        else if (!strncmp(key, "comment", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            printf("comment: %.*s\n", len, val);
        }
        else if (!strncmp(key, "created by", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            printf("created by: %.*s\n", len, val);
        }
        else if (!strncmp(key, "creation date", klen))
        {
            long int date;

            bencode_int_value(&benk, &date);
            printf("created date: %ld\n", date);
        }
        else if (!strncmp(key, "encoding", klen))
        {
            int len;

            const char *val;

            bencode_string_value(&benk, &val, &len);
            printf("encoding: %.*s\n", len, val);
        }
        else if (!strncmp(key, "info", klen))
        {

//            printf("doing info:%s\n", benk.str);
            __do_info_dict(id, &benk);
        }
    }
}
