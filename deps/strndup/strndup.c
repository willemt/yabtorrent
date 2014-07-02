
#include <stdlib.h>
#include <string.h>

char *strndup(const char *s, size_t n)
{
    char* new = malloc(n+1);
    strncpy(new, s, n);
    new[n] = '\0';
    return new;
}
