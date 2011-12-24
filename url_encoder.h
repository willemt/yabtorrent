/*
 * =====================================================================================
 *
 *       Filename:  url_encoder.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/15/11 18:33:34
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */

char from_hex(
    char ch
);

char to_hex(
    char code
);

char *url_encode(
    const char *str
);

char *url_decode(
    const char *str
);
