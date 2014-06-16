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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#if WIN32

#else
#include <sys/mman.h>
#endif
#include <unistd.h>
#include "list.h"
#include "config.h"


typedef struct {
	char *key;
	char *val;
	char *desc;
	int   locked;
} configitem;

static void config_set_strerror(config_t* cfg, char *fmt, ...);

int config_set(config_t* cfg, const char *key, const char *val)
{
	return config_set_with_desc(cfg, key, val, NULL);
}

int config_set_if_not_set(config_t* cfg, const char *key, const char *val)
{
    if (!config_get(cfg,key))
        return config_set_with_desc(cfg, key, val, NULL);
    else return 0;
}

int config_set_va(config_t* cfg, const char *key, const char *val, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, val);
    vsprintf(buf, val, args);

    return config_set(cfg, key, buf);
}

int config_set_with_desc(config_t* cfg, const char *key, const char *val, const char *desc)
{
	char *keybuf, *valbuf, *descbuf = NULL;
	configitem *item;
	node_l *n;

	assert(key != NULL);
	assert(val != NULL);

	// check whether key already exists
	n = list_get_first_node((node_l**)&cfg->config);
	while (n != NULL) {
		item = (configitem *)(n->data);
		if (!strcmp(key, item->key)) {
			if (item->locked) return 0;
			else break;
		}

		n = list_get_next_node((node_l**)&cfg->config, n);
	}

	if (n != NULL) {  // key already exists
		if (item->locked == 1)
			return -1;

		keybuf = item->key;
		descbuf = item->desc;

		// update value?
		if (strcmp(item->val, val)) {
			free(item->val);
			valbuf = strdup(val);
			if (valbuf == NULL)
				return -1;
		} else {
			valbuf = item->val;
		}
	} else {  // key doesn't exist, create it
		keybuf = strdup(key);
		if (keybuf == NULL)
			return -1;

		valbuf = strdup(val);
		if (valbuf == NULL) {
			free (keybuf);
			return -1;
		}

		if (desc) {
			descbuf = strdup(desc);
			if (descbuf == NULL) {
				free (keybuf);
				free (valbuf);
				return -1;
			}
		} else descbuf = NULL;

		item = malloc(sizeof(*item));
		if (item == NULL) {
			free (keybuf);
			free (valbuf);
			free (descbuf);
			return -1;
		}

		item->locked = 0;
		list_prepend((node_l**)&cfg->config, item);
	}

	item->key = keybuf;
	item->val = valbuf;
	item->desc = descbuf;

	return 0;
}

static node_l *search_node(config_t* cfg, const char *key)
{
	node_l *n;
	configitem *item;

	n = list_get_first_node((node_l**)&cfg->config);

	while (n != NULL) {
		item = n->data;
		if (!strcmp(key, item->key))
			return n;

		n = list_get_next_node((node_l**)&cfg->config, n);
	}

	return NULL;
}

void config_lock(config_t* cfg, const char *key)
{
	node_l *n;
	configitem *item;

	n = search_node(cfg,key);
	if (n != NULL) {
		item = n->data;
		item->locked = 1;
	}
}

char *config_get(config_t* cfg, const char *key)
{
	node_l *n;
	configitem *item;

        if (!cfg)
            return NULL;

	n = search_node(cfg,key);
	if (n != NULL) {
		item = n->data;
		return item->val;
	}

	return NULL;
}

int config_get_int(config_t* cfg, const char *key)
{
    char* val;

    if (!cfg) return 0;

    if ((val = config_get(cfg,key)))
    {
        return atoi(val);
    }

    return 0;
}

static int config_set_with_level(config_t* cfg, const char *level, const char *key, const char *val)
{
	char *tmp;
	int ret;

	assert(key != NULL);
	assert(val != NULL);

	if (level == NULL) {
		return config_set(cfg, key, val);
	}

	tmp = malloc(strlen(level) + strlen(key) + 2);
	if (tmp == NULL)
		return -1;

	sprintf(tmp, "%s.%s", level, key);
	ret = config_set(cfg, tmp, val);
	free(tmp);

	return ret;
}

static char *levelup(char *cur, const char *next)
{
	size_t len;

	if (cur == NULL) {
		len = strlen(next) + 1;
		cur = malloc(len);
		if (cur == NULL)
			return NULL;
		strcpy(cur, next);
	} else {
		len = strlen(cur) + strlen(next) + 2;
		cur = realloc(cur, len);
		if (cur == NULL)
			return NULL;
		strcat(cur, ".");
		strcat(cur, next);
	}

	return cur;
}

static char *leveldown(char *level)
{
	char *p;

	if (level == NULL)
		return NULL;

	p = strrchr(level, '.');

	if (p == NULL) {
		free(level);
		level = NULL;
	} else {
		*p = '\0';
	}

	return level;
}

static char *addchar(char *str, int c)
{
	int len;

	len = (str == NULL) ? 1 : strlen(str) + 1;

	if (len == 1)
		str = malloc(2);
	else if ((len & (len - 1)) == 0)     /* if len is a power of 2 */
		str = realloc(str, len<<1);  /* allocate memory for the next power of 2 */

	if (str == NULL)
		return NULL;

	str[len - 1] = c;
	str[len] = '\0';

	return str;
}

const char *config_strerror(config_t* cfg)
{
	return cfg->errmsg;
}

static void config_set_strerror(config_t* cfg, char *fmt, ...)
{
        va_list argp;

        va_start(argp, fmt);
        vsnprintf(cfg->errmsg, sizeof(cfg->errmsg), fmt, argp);
        va_end(argp);

        cfg->errmsg[sizeof(cfg->errmsg) - 1] = '\0';
}

static int filesize(config_t* cfg, FILE *fp)
{
	long cur;
	int size = -1;

	cur = ftell(fp);
	if (cur < 0)
		goto filesize_error;

	if (fseek(fp, 0, SEEK_END) < 0)
		goto filesize_error;

	size = ftell(fp) - cur;
	if (size < 0)
		goto filesize_error;

	rewind(fp);
	if (fseek(fp, cur, 0) < 0)
		goto filesize_error;

	return size;

filesize_error:
	config_set_strerror(cfg, "filesize(): %s", strerror(errno));
	return -1;
}

int config_read_fp(config_t* cfg, FILE *fp)
{
	int size, fd;

	size = filesize(cfg,fp);

	fd = fileno(fp);
	if (fd < 0) {
		config_set_strerror(cfg, "config_read_fp(): %s", strerror(errno));
		return -1;
	}

#if WIN32
        /* TODO: not supported on Windows */
        return 0;
#else
        void* mem;
	mem = mmap(0, size, PROT_READ, MAP_SHARED, fd, ftell(fp));
	if (mem == MAP_FAILED) {
		config_set_strerror(cfg, "config_read_fp(): %s", strerror(errno));
		return -1;
	}

	return config_read_mem(cfg, mem, size);
#endif
}

int config_read_mem(config_t* cfg, char *mem, size_t n)
{
	int c, line, next, status;
	enum {prekey, key, postkey, preval, val_raw, val_single, val_double, postval, comment} state = prekey;
	char *buf, *keybuf, *valbuf, *level;
	size_t i;

	line = 1;
	buf = keybuf = valbuf = level = NULL;

	for (i = 0; i <= n; ++i) {
	c = *mem++;
	if (i == n) c = EOF;
	switch (c) {
		case ';':
			switch (state) {
				case prekey:
				case val_raw:
				case postval:
					while (c != '\n' && c != '\r' && c != EOF && i < n)
						++i, c = *mem++;
					--i, --mem;
					break;
				default:
					config_set_strerror(cfg, "unexpected start of comment");
					goto error;
			}
			break;
		case '{':
			switch (state) {
				case key:
				case postkey:
					level = levelup(level, buf);
					free(buf);
					buf = NULL;
					state = prekey;
					break;
				default:
					config_set_strerror(cfg, "unexpected level declaration");
					goto error;
			}
			break;
		case '}':
			switch (state) {
				case prekey:
				case val_raw:
				case postval:
					if (level == NULL)
						return line;

					level = leveldown(level);
					break;
				default:
					config_set_strerror(cfg, "unexpected '}'");
					goto error;
			}
			break;
		case '=':
			switch (state) {
				case key:
				case postkey:
					keybuf = strdup(buf);
					free(buf);
					buf = NULL;
					state = preval;
					break;
				default:
					config_set_strerror(cfg, "unexpected '='");
					goto error;
			}
			break;
		case '"':
			switch (state) {
				case preval:
					state = val_double;
					break;
				case val_double:
					state = postval;
					break;
				case val_single:
					buf = addchar(buf, c);
					break;
				default:
					config_set_strerror(cfg, "unexpected '\"'");
					goto error;
			}
			break;
		case '\'':
			switch (state) {
				case preval:
					state = val_single;
					break;
				case val_single:
					state = postval;
					break;
				case val_double:
					buf = addchar(buf, c);
					break;
				default:
					config_set_strerror(cfg,"unexpected \"'\"");
					goto error;
			}
			break;
		case EOF:
			switch (state) {
				case prekey:
				case postval:
				case val_raw:
					break;
				default:
					line = -1;
					config_set_strerror(cfg,"parse error");
					goto error;
			}
		case '\r':
		case '\n':
			switch (state) {
				case prekey:
					break;
				case val_raw:
				case postval:
					valbuf = strdup(buf);
					free(buf);
					buf = NULL;

					if (keybuf && valbuf) {
						status = config_set_with_level(cfg,level, keybuf, valbuf);
						free(keybuf);
						free(valbuf);

						keybuf = valbuf = NULL;

						if (status) {
							config_set_strerror(cfg,"out of memory");
							line = -1;
							goto error;
						}
					} else {
						free (keybuf);
						free (valbuf);

						config_set_strerror(cfg,"out of memory");
						line = -1;
						goto error;
					}

					break;
				default:
					config_set_strerror(cfg,"unexpected end of file");
					goto error;
			}

			++line;
			state = prekey;

			break;
		case ' ':
		case '\t':
			switch (state) {
				case key:
					state = postkey;
					break;
				case val_raw:
					state = postval;
					break;
				case val_single:
				case val_double:
					buf = addchar(buf, c);
					break;
				default:
					continue;
			}
			break;
		case '\\':
			//next = fgetc(fp);
			next = *(mem+1);

			switch (state) {
				case val_single:
					if (next == '\'') {
						buf = addchar(buf, '\'');
						++mem, ++i;
					} else {
						buf = addchar(buf, '\\');
					}
					break;
				case val_double:
					++mem, ++i;
					if (next == 'n')       buf = addchar(buf, '\n');
					else if (next == 't')  buf = addchar(buf, '\t');
					else if (next == 'r')  buf = addchar(buf, '\r');
					else if (next == 'v')  buf = addchar(buf, '\v');
					else if (next == '\\') buf = addchar(buf, '\\');
					else --mem, --i;
					break;
				default:
					break;
			}
			break;
		default:
			switch (state) {
				case prekey:
					state = key;
				case key:
					buf = addchar(buf, c);
					break;
				case preval:
					state = val_raw;
				case val_raw:
				case val_single:
				case val_double:
					buf = addchar(buf, c);
					break;
				default:
					config_set_strerror(cfg,"parse error");
					goto error;
			}
		}
	}

	if (level != NULL) {
		config_set_strerror(cfg,"missing '}'");
		goto error;
	} 

	return 0;

error:
	free(buf);
	free(keybuf);
	free(valbuf);
	free(level);

	return line;
}

int config_read_file(config_t* cfg, const char *path)
{
	FILE *fp;
	int ret;

	if ((fp = fopen(path, "r")) == NULL) {
		config_set_strerror(cfg,"config_read_file(): \"%s\": %s", path, strerror(errno));
		return -1;
	}

	ret = config_read_fp(cfg, fp);

	fclose(fp);

	return ret;
}

int config_write_file(config_t* cfg, const char *path)
{
	FILE *fp;
	configitem *citem;
	node_l *n;

	if((fp = fopen(path, "wb")) != NULL) {
		n = list_get_first_node((node_l**)&cfg->config);

		while (n != NULL) {
			citem = (configitem *)n->data;

			if (citem->desc != NULL)
				fprintf(fp, "\n; %s\n", citem->desc);

			fprintf(fp, "%s = %s\n", citem->key, citem->val!=NULL?citem->val:"");

			n = list_get_next_node((node_l**)&cfg->config, n);
		}
	} else {
		config_set_strerror(cfg,"%s: %s", path, strerror(errno));
		return -1;
	}
	
	return 0;
}

int config_get_default_path(config_t* cfg, char *path, size_t size, const char *dir, const char *file)
{
	char *home, *xdg;
	int nchars;

	home = getenv("HOME");
	xdg = getenv("XDG_CONFIG_HOME");

	if (xdg) {
		nchars = snprintf(path, size, "%s/%s/%s", xdg, dir, file);
	} else {
		nchars = snprintf(path, size, "%s/.config/%s/%s", home, dir, file);
	}

	if (nchars < 0) {
		config_set_strerror(cfg,"config_get_default_path(): %s",strerror(errno));
		return -1;
	}

	if ((unsigned int)nchars >= size) {
		config_set_strerror(cfg,"config_get_default_path(): %s", strerror(ENAMETOOLONG));
		return -1;
	}

	return 0;
}

static int mkdirs(const char *path, mode_t mode)
{
	char buf[BUFSIZ];
	char *p;
	char const *str;

	if (strlen(path) >= BUFSIZ)
		return -1;

	str = path;
//	printf("path: %s\n", path);

	while ((p = strchr(str, '/')) != NULL) {
		if (p - str) {
			strncpy(buf, path, p - path + 1);
			buf[p - path + 1] = '\0';
#if WIN32
			if (mkdir(buf) < 0)
#else
			if (mkdir(buf, mode) < 0)
#endif
				if (errno != EEXIST)
					return -1;
		}
		str = p + 1;
	}

	return 0;
}

int config_write(config_t* cfg, const char *dir, const char *file)
{
	char path[BUFSIZ];

	if (config_get_default_path(cfg,path, BUFSIZ, dir, file) < 0) {
		config_set_strerror(cfg,"config_create_default(): %s\n", strerror(errno));
		return -1;
	}

	if (mkdirs(path, S_IRWXU) < 0) {
		config_set_strerror(cfg,"config_create_default(): \"%s\": %s\n", path, strerror(errno));
		return -1;
	}

	return config_write_file(cfg, path);
}

int config_read(config_t* cfg, const char *dir, const char *file)
{
	char path[BUFSIZ];

	if (config_get_default_path(cfg, path, BUFSIZ, dir, file) < 0)
		return -1;

	return config_read_file(cfg, path);
}

void config_print(config_t* cfg)
{
	node_l *n;
	configitem *item;

	n = list_get_first_node((node_l**)&cfg->config);

	while (n != NULL) {
		item = n->data;

		if (item->desc)
			printf("\n; %s\n", item->desc);
		printf("%s = \"%s\"\n", item->key, item->val);

		n = list_get_next_node((node_l**)&cfg->config, n);
	}
}

static void configitem_free(void *data)
{
	configitem *item = data;

	if (item == NULL)
		return;

	free(item->key);
	free(item->val);
	free(item);
}

void config_free(config_t* cfg)
{
	if (cfg->config == NULL)
		return;

	list_foreach((node_l**)&cfg->config, configitem_free);
	list_destroy((node_l**)&cfg->config);
	cfg->config = NULL;
}

config_t* config_new()
{
    config_t* cfg;
    cfg = calloc(1,sizeof(config_t));
//    cfg->config = list_alloc_node(NULL);
    return cfg;
}

/*
int main(int argc, char *argv[])
{
	int status;

	if (argc != 3) {
		fprintf(stderr, "USAGE: %s <dir> <file>\n", argv[0]);
		return -1;
	}

	//config_set_with_lock("foo", "bar", 0);
	//config_set_with_desc("tra", "23", "long description");
	config_set("foo", "bla");

	status = config_write(argv[1], argv[2]);
	if (status != 0)
		printf("%s: line %d: %s\n", argv[1], status, config_strerror());

	status = config_read(argv[1], argv[2]);
	if (status != 0)
		printf("%s: line %d: %s\n", argv[1], status, config_strerror());

	config_print();

	config_free();

	return 0;
}
*/
