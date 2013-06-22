#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* for vargs */
#include <stdarg.h>

#include "bt_string.h"

#if WIN32
int asprintf(char **resultp, const char *format, ...)
{
    char buf[1024];
    va_list args;

    va_start (args, format);
    vsprintf(buf, format, args);
    *resultp = strdup(buf);
    va_end (args);
    return 1;
}

char* strndup(const char* str, const unsigned int len)
{
    char* new;

    new = malloc(len+1);
    strncpy(new,str,len);
    return new;
}
#endif

