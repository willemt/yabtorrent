#ifndef BITFIELD_H
#define BITFIELD_H

typedef struct
{
    unsigned char *bits;
    /* size in number of bits */
    unsigned int size;
} bitfield_t;

/**
 * Allocate and initialise a bitfield */
bitfield_t* bitfield_new(const unsigned int nbits);

/**
 * Initialise a bitfield
 * @param nbits size of bitfield in bits */
void bitfield_init(bitfield_t * bf, const unsigned int nbits);

/**
 * Copy the bitfield into another bitfield. */
void bitfield_clone(bitfield_t * bf, bitfield_t * clone);

/**
 * Free all memory (including bitfield_t*) */
void bitfield_free(bitfield_t* bf);

/**
 * Mark bit as on. */
void bitfield_mark(bitfield_t * bf, const unsigned int bit);

/**
 * Mark bit as off. */
void bitfield_unmark(bitfield_t * bf, const unsigned int bit);

/**
 * @return 1 if that bit is marked as on.  */
int bitfield_is_marked(bitfield_t * me, const unsigned int bit);

/**
 * @return size of bitfield in bits */
unsigned int bitfield_get_length(bitfield_t * bf);

/**
 * Output bitfield to new string
 * @return string representation of the bitfield (string is null terminated) */
char *bitfield_str(bitfield_t * bf);

#endif /* BITFIELD_H */
