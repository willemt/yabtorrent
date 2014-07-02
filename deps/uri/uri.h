
/**
 * `uri.h' - uri.c
 *
 * copyright (c) 2014 joseph werle
 */

#ifndef URI_H
#define URI_H 1

/**
 * Encodes a URI component by replacing each
 * instance of certain characters by one, two, three, or four escape
 * sequences representing the UTF-8 encoding of the character
 */

char *
uri_encode (const char *);

/**
 * Decodes a URI component source from `uri_encode'
 */

char *
uri_decode (const char *);

#endif
