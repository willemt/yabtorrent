
/**
 * `encode.c' - uri.c
 *
 * copyright (c) 2014 joseph werle
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "uri.h"

#define IN_URANGE(a,b,c) (a >= (unsigned int) b && a <= (unsigned int) c)

static inline int
needs_encoding (char ch, char next) {
  if (IN_URANGE(ch, 0xD800, 0xDBFF)) {
    if (!IN_URANGE(next, 0xDC00, 0xDFFF)) {
      return -1;
    }
  }

  // alpha capital/small
  if (IN_URANGE(ch, 0x0041, 0x005A) ||
      IN_URANGE(ch, 0x061, 0x007A)) {
    return 0;
  }

  // decimal digits
  if (IN_URANGE(ch, 0x0030, 0x0039)) {
    return 0;
  }

  // reserved chars
  // - _ . ! ~ * ' ( )
  if ('-' == ch || '_' == ch || '.' == ch ||
      '!' == ch || '~' == ch || '*' == ch ||
      '(' == ch || ')' == ch ) {
    return 0;
  }

  return 1;
}

char *
uri_encode (const char *src) {
  int i = 0;
  size_t size = 0;
  size_t len = 0;
  size_t msize = 0;
  char *enc = NULL;
  char tmp[4];
  char ch = 0;

  // chars length
  len = strlen(src);

#define push(c) (enc[size++] = c)

  for (; i < len; ++i) {
    switch (needs_encoding(src[i], src[i+1])) {
      case -1:
        // @TODO - handle with uri error
        free(enc);
        return NULL;

      case 0:
        msize++;
        break;

      case 1:
        msize = (msize + 3); // %XX
        break;
    }
  }

  // alloc with probable size
  enc = (char *) malloc(sizeof(char) * msize);
  if (NULL == enc) { return NULL; }

  // reset
  i = 0;

  // encode
  while (len--) {
    ch = src[i++];
    // if high surrogate ensure next is
    // low surrogate for valid surrogate
    // pair
    if (needs_encoding(ch, src[i])) {
      // encode
      sprintf(tmp, "%%%x", ch & 0xff);

      // prefix
      push(tmp[0]);

      // codec
      push(toupper(tmp[1]));
      push(toupper(tmp[2]));
    } else {
      push(ch);
    }
  }

  enc[size] = '\0';

#undef push

  return enc;
}

#undef IN_URANGE
