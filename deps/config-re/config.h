/*  Copyright (c) 2011, Philip Busch <vzxwco@gmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    void *config;
    char errmsg[BUFSIZ];
} config_t;


config_t* config_new();
int   config_read(config_t* cfg, const char *dir, const char *file);
int   config_read_file(config_t* cfg, const char *path);
int   config_read_fp(config_t* cfg, FILE *fp);
int   config_read_mem(config_t* cfg, char *mem, size_t n);
int   config_set_if_not_set(config_t* cfg, const char *key, const char *val);
int   config_set(config_t* cfg, const char *key, const char *val);
int   config_set_va(config_t* cfg, const char *key, const char *val, ...);
int   config_set_with_desc(config_t* cfg, const char *key, const char *val, const char *desc);
void  config_lock(config_t* cfg, const char *key);
char *config_get(config_t* cfg, const char *key);
int   config_get_int(config_t* cfg, const char *key);
void  config_print(config_t* cfg);
void  config_free(config_t* cfg);
int   config_get_default_path(config_t* cfg, char *buf, size_t n, const char *dir, const char *file);
const char *config_strerror(config_t* cfg);

#endif  /* ! CONFIG_H */
