
/**
 * `decode.c' - uri
 *
 * copyright (c) 2014 joseph werle
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "uri.h"

char *
uri_decode (const char *src) {
  int i = 0;
  size_t size = 0;
  size_t len = 0;
  char *dec = NULL;
  char tmp[3];
  char ch = 0;

  // chars len
  len = strlen(src);

  // alloc
  dec = (char *) malloc(len);

#define push(c) (dec[size++] = c)

  // decode
  while (len--) {
    ch = src[i++];

    // if prefix `%' then read byte and decode
    if ('%' == ch) {
      tmp[0] = src[i++];
      tmp[1] = src[i++];
      tmp[2] = '\0';
      push(strtol(tmp, NULL, 16));
    } else {
      push(ch);
    }
  }

  dec[size] = '\0';

#undef push

  return dec;
}
