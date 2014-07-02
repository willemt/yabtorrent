
/**
 * @file
 * @brief Read bittorrent meta files 
 * @author  Willem Thiart himself@willemthiart.com
 * @version 0.1
 *
 * @section LICENSE
 * Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bencode.h"
#include "torrent_reader.h"

typedef struct {
    int (*cb_event)(void* udata, const char* key);
    int (*cb_event_str)(void* udata, const char* key, const char* val, int len);
    int (*cb_event_int)(void* udata, const char* key, int val);
    void* udata;
} tfr_t;

#if WIN32
static char* strndup(const char* str, const unsigned int len)
{
    char* new;

    new = malloc(len+1);
    strncpy(new,str,len);
    return new;
}
#endif

void tfr_event(tfr_t* me, const char* key)
{
    me->cb_event(me->udata,key);
}

void tfr_event_str(tfr_t* me, const char* key, const char* val, int len)
{
    me->cb_event_str(me->udata,key,val,len);
}

void tfr_event_int(tfr_t* me, const char* key, int val)
{
    me->cb_event_int(me->udata,key,val);
}

static void __do_files_list(
    tfr_t* me,
    bencode_t * list
)
{
    while (bencode_list_has_next(list))
    {
        bencode_t dict;

        bencode_list_get_next(list, &dict);
        tfr_event(me, "file");

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
                long int current_file_len = 0;

                bencode_int_value(&benk, &current_file_len);
                tfr_event_int(me, "file length", current_file_len);
            }
            /*
             * 'md5sum':
             * This is an OPTIONAL value.if included
             * it must be a string of 32 characters corresponding to the MD5
             * sum of the file.This value is not used in BTP / 1.0.
             */
            else if (!strncmp(key, "md5sum", klen))
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
                char *fullpath = NULL;

                /* count including last nul byte */
                int fp_len = 0; 

                while (bencode_list_has_next(&benk))
                {
                    const char *path;

                    bencode_t pathitem;

                    bencode_list_get_next(&benk, &pathitem);
                    bencode_string_value(&pathitem, &path, &klen);

                    if (!fullpath)
                    {
                        fullpath = strndup(path, klen);
                        fp_len = klen;
                    }
                    else
                    {
                        fullpath = realloc(fullpath, fp_len + klen + 2);
                        fullpath[fp_len] = '/';
                        strncpy(fullpath + fp_len + 1, path, klen);
                        fp_len += klen + 1;
                    }
                }

                tfr_event_str(me, "file path", fullpath, fp_len);
                free(fullpath);
            }
        }
    }
}

static void __do_info_dict(
    tfr_t* me,
    bencode_t * dict
)
{
    {
        const char *val;
        int len;

        bencode_dict_get_start_and_len(dict, &val, &len);
        tfr_event_str(me, "infohash", val, len);
    }

    while (bencode_dict_has_next(dict))
    {
        const char *key;
        int klen;
        bencode_t benk;

        bencode_dict_get_next(dict, &benk, &key, &klen);

        /* is a single file torrent */
        if (!strncmp(key, "length", klen))
        {
            long int file_len;

            bencode_int_value(&benk, &file_len);
            bencode_dict_get_next(dict, &benk, &key, &klen);
            if (!strncmp(key, "name", klen))
            {
                int len;
                const char *name;

                bencode_string_value(&benk, &name, &len);
                tfr_event(me, "file");
                tfr_event_str(me, "file path", name, len);
                tfr_event_int(me, "file length", file_len);
            }
        }
        /*  ...otherwise multi-file torrent */
        else if (!strncmp(key, "files", klen))
        {
            __do_files_list(me, &benk);
        }
        else if (!strncmp(key, "piece length", klen))
        {
            long int piece_len;

            piece_len = 0;
            bencode_int_value(&benk, &piece_len);
            tfr_event_int(me, "piece length", piece_len);
        }
        else if (!strncmp(key, "pieces", klen))
        {
            int len;
            const char *val;

            bencode_string_value(&benk, &val, &len);
            tfr_event_str(me, "pieces", val, len);
        }
    }
}

void tfr_read_metainfo(
    void *me,
    const char *buf,
    const int len
)
{
    bencode_t ben;

    bencode_init(&ben, buf, len);

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

        if (!strncmp(key, "announce", klen))
        {
            int len;
            const char *val;

            bencode_string_value(&benk, &val, &len);
            tfr_event_str(me, "announce", val, len);
        }
        else if (!strncmp(key, "announce-list", klen))
        {
            /*  loop through announce list */

            assert(bencode_is_list(&benk));

            while (bencode_list_has_next(&benk))
            {
                bencode_t innerlist;

                bencode_list_get_next(&benk, &innerlist);
                while (bencode_list_has_next(&innerlist))
                {
                    bencode_t benlitem;
                    const char *backup;
                    int len;

                    bencode_list_get_next(&innerlist, &benlitem);
                    bencode_string_value(&benlitem, &backup, &len);
                    tfr_event_str(me, "tracker_backup", backup, len);
                }
            }
        }
        else if (!strncmp(key, "comment", klen))
        {
            int len;
            const char *val;

            bencode_string_value(&benk, &val, &len);
        }
        else if (!strncmp(key, "created by", klen))
        {
            int len;
            const char *val;

            bencode_string_value(&benk, &val, &len);
        }
        else if (!strncmp(key, "creation date", klen))
        {
            long int date;

            bencode_int_value(&benk, &date);
        }
        else if (!strncmp(key, "encoding", klen))
        {
            int len;
            const char *val;

            bencode_string_value(&benk, &val, &len);
        }
        else if (!strncmp(key, "info", klen))
        {
            __do_info_dict(me, &benk);
        }
    }
}

void* tfr_new(
    int (*cb_event)(void* udata, const char* key),
    int (*cb_event_str)(void* udata, const char* key, const char* val, int len),
    int (*cb_event_int)(void* udata, const char* key, int val),
    void* udata
    )
{
    tfr_t * me;

    me = calloc(1,sizeof(tfr_t));
    me->cb_event = cb_event;
    me->cb_event_str = cb_event_str;
    me->cb_event_int = cb_event_int;
    me->udata = udata;
    return me;
}
