
#include <stdint.h>
#include <string.h>
#include <assert.h>

void bitstream_init(
    unsigned char *b,
    int size
)
{
    memset(b, 0, sizeof(unsigned char) * size);
}

void bitstream_write_ubyte(
    unsigned char **b,
    unsigned char value
)
{
    **b = value;
    *b += 1;
}

void bitstream_write_uint32(
    unsigned char **b,
    uint32_t value
)
{
    memcpy(*b, &value, sizeof(uint32_t));
    *b += sizeof(uint32_t);
}

unsigned char bitstream_read_ubyte(
    unsigned char **b
)
{
    unsigned char val = **b;
    *b += 1;
    return val;
}

uint32_t bitstream_read_uint32(
    unsigned char **b
)
{
    uint32_t value;

    memcpy(&value, *b, sizeof(uint32_t));
    *b += sizeof(uint32_t);
    return value;
}

void bitstream_write_uint32_from_bitoffset(
    unsigned char **b,
    const uint32_t val,
    const unsigned int nbits,
    unsigned int* bit_pos
)
{
    unsigned int bit_offset, int_offset;
    uint32_t val_posting;

    assert(nbits <= 32);

    int_offset = (*bit_pos - *bit_pos % 32) / 32;
    bit_offset = *bit_pos % 32;

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
    unsigned char **b,
    const unsigned int val,
    unsigned int* bit_pos
)
{
    bitstream_write_uint32_from_bitoffset(b,val,1,bit_pos);
}

void bitstream_read_uint32_from_bitoffset(
    unsigned char **b,
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

/**
 * read the next bit.
 * @return true if bit == 1 else false
 */
int bitstream_read_bit(
    unsigned char **b,
    unsigned int* bit_pos
)
{
    uint32_t *ptr;
    uint32_t val;
    unsigned int bit_offset, int_offset;

    /* position pointer */
    int_offset = (*bit_pos - *bit_pos % 32) / 32;
    bit_offset = *bit_pos % 32;
    ptr = (uint32_t*)*b + int_offset;

    //val = ntohl(*ptr);
    val = *ptr;
    val <<= bit_offset;
    val >>= 32 - 1;
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
    unsigned char **b,
    const char* string,
    unsigned int len
)
{
    strncpy((char*)*b,string,len);
    *b += len;
}

void bitstream_read_string(
    unsigned char **b,
    char* out_string,
    unsigned int len
)
{
    strncpy(out_string,(char*)*b, len);
    *b += len;
}

