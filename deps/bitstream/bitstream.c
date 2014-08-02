
#include <stdint.h>
#include <string.h>
#include <assert.h>

void bitstream_init(
    char *b,
    int size
)
{
    memset(b, 0, sizeof(char) * size);
}

void bitstream_write_byte(
    char **b,
    char value
)
{
    **b = value;
    *b += 1;
}

void bitstream_write_byte_from_bitoffset(
    char **b,
    char value,
    unsigned int* bit_pos
)
{
    /* position pointer */
    unsigned int byte_offset = (*bit_pos - *bit_pos % 8) / 8;
    unsigned int bit_offset = *bit_pos % 8;
    unsigned char *ptr = (unsigned char*)*b + byte_offset;

    /* left half */
    unsigned char val = *ptr;
    val >>= bit_offset;
    val <<= bit_offset;
    val |= value >> bit_offset;
    *ptr = val;

    /* right half */
    val = value << bit_offset;
    *(ptr + 1) = val;
    *b += 1;
}

void bitstream_write_uint32(
    char **b,
    uint32_t value
)
{
    memcpy(*b, &value, sizeof(uint32_t));
    *b += sizeof(uint32_t);
}

char bitstream_read_byte(
    char **b
)
{
    unsigned char val = **b;
    *b += 1;
    return val;
}

uint32_t bitstream_read_uint32(
    char **b
)
{
    uint32_t value;

    memcpy(&value, *b, sizeof(uint32_t));
    *b += sizeof(uint32_t);
    return value;
}

void bitstream_write_uint32_from_bitoffset(
    char **b,
    const uint32_t val,
    const unsigned int nbits,
    unsigned int* bit_pos
)
{

    assert(nbits <= 32);

    unsigned int int_offset = (*bit_pos - *bit_pos % 32) / 32;
    unsigned int bit_offset = *bit_pos % 32;

    uint32_t val_posting;
    /* get old value */
    memcpy(&val_posting, *b + (int_offset) * sizeof(uint32_t), sizeof(uint32_t));

    /* write b out */
    val_posting = val_posting | ((val << (32 - nbits)) >> bit_offset);
    //memcpy(*b, &val_posting, sizeof(uint32_t));
    memcpy(*b + (int_offset) * sizeof(uint32_t), &val_posting, sizeof(uint32_t));

    /* do right handside */
    if (32 < bit_offset + nbits)
    {
        int nbits2 = (bit_offset + nbits) % 32;
        val_posting = val << (32 - nbits2);
        //memcpy(*b, &val_posting, sizeof(uint32_t));
        memcpy(*b + (int_offset + 1) * sizeof(uint32_t), &val_posting, sizeof(uint32_t));
    }

    *bit_pos += nbits;
}

void bitstream_write_bit_from_bitoffset(
    char **b,
    const unsigned int val,
    unsigned int* bit_pos
)
{
    /* position pointer */
    unsigned int byte_offset = (*bit_pos - *bit_pos % 8) / 8;
    unsigned int bit_offset = *bit_pos % 8;
    unsigned char *ptr = (unsigned char*)*b + byte_offset;

    unsigned char stamp;

    stamp = val != 0;
    stamp <<= 8 -1;
    stamp >>= bit_offset;
    *ptr |= stamp;
    *bit_pos += 1;
}

void bitstream_read_uint32_from_bitoffset(
    char **b,
    uint32_t * val_out,
    const unsigned int nbits,
    unsigned int* bit_pos
)
{
    unsigned int bit_offset, int_offset;

    assert(nbits <= 32);

    /* position pointer */
    int_offset = (*bit_pos - *bit_pos % 32) / 32;
    bit_offset = *bit_pos % 32;

    /* read b */
    //*val = ntohl(*ptr);
    //*val = l2b_endian(*ptr);
    memcpy(val_out, *b + int_offset * sizeof(uint32_t), sizeof(uint32_t));
    *val_out <<= bit_offset;
    *val_out >>= 32 - nbits;

    /* do otherside */
    if (32 < bit_offset + nbits)
    {
        int nbits2;
        uint32_t val;

        nbits2 = (bit_offset + nbits) % 32;
        memcpy(&val, *b + (int_offset + 1) * sizeof(uint32_t), sizeof(uint32_t));

        //*val |= ntohl(*ptr) >> (32 - nbits2);
        //*val |= l2b_endian(*ptr) >> (32 - nbits2);
        //*val_out |= *ptr >> (32 - nbits2);
        *val_out |= val >> (32 - nbits2);
    }

    *bit_pos += nbits;
}

int bitstream_read_bit(
    char **b,
    unsigned int* bit_pos
)
{
    /* position pointer */
    unsigned int byte_offset = (*bit_pos - *bit_pos % 8) / 8;
    unsigned int bit_offset = *bit_pos % 8;
    unsigned char *ptr = (unsigned char*)*b + byte_offset;

    //val = ntohl(*ptr);
    unsigned char val = *ptr;
    val <<= bit_offset;
    val >>= 8 - 1;
    *bit_pos += 1;
    return val == 1;
}

#if 0
static int l2b_endian(int in)
{
    int out;

    ((unsigned char*)&out)[0] = ((unsigned char*)&in)[3];
    ((unsigned char*)&out)[1] = ((unsigned char*)&in)[2];
    ((unsigned char*)&out)[2] = ((unsigned char*)&in)[1];
    ((unsigned char*)&out)[3] = ((unsigned char*)&in)[0];

    return out;
}
#endif

void bitstream_write_string(
    char **b,
    const char* string,
    unsigned int len
)
{
    strncpy((char*)*b,string,len);
    *b += len;
}

void bitstream_read_string(
    char **b,
    char* out_string,
    unsigned int len
)
{
    strncpy(out_string,(char*)*b, len);
    *b += len;
}

