
#include <stdint.h>

typedef unsigned char byte;

void stream_write_ubyte(
    unsigned char **bytes,
    unsigned char value
)
{
//    char *byte;

    **bytes = value;
    *bytes += 1;
}

void stream_write_uint32(
    unsigned char **bytes,
    uint32_t value
)
{
    uint32_t *ptr;

    ptr = (uint32_t *) (*bytes);

    *ptr = htonl(value);

    *bytes += 4;
}

unsigned char stream_read_ubyte(
    unsigned char **bytes
)
{
    unsigned char val;

//    char *byte;

//    byte = *bytes;
    val = **bytes;
    *bytes += 1;

    return val;
}

uint32_t stream_read_uint32(
    unsigned char **bytes
)
{
    uint32_t *ptr;

    ptr = (uint32_t *) (*bytes);

    *bytes += 4;

    return ntohl(*ptr);
}
