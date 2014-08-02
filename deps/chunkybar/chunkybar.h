#ifndef CHUNKYBAR_H
#define CHUNKYBAR_H

typedef struct {
    void *first_chunk;
    unsigned int max;
} chunkybar_t;

/**
 * @param max The amount we are expecting to count to
 * @return initialised counter */
void *chunky_new(const unsigned int max);

/**
 * @param max The amount we are expecting to count to */
void chunky_set_max(chunkybar_t* me, const unsigned int max);

/**
 * Free all memory held */
void chunky_free(void *ra);

/**
 * @return number of non-contiginous chunks */
int chunky_get_num_chunks(
        const chunkybar_t * prog);

/**
 * Mark this chunk as complete */
void chunky_mark_complete(
        chunkybar_t * prog,
        const unsigned int offset,
        const unsigned int len);

/**
 * Mark this chunk as incomplete */
void chunky_mark_incomplete(
        chunkybar_t * prog,
        const unsigned int offset,
        const unsigned int len);

/**
 * Mark everything as incomplete */
void chunky_mark_all_incomplete(chunkybar_t * me);

/** 
 * Is everything complete? */
int chunky_is_complete(
        const chunkybar_t * prog);

/**
 * Get an incomplete chunk  */
void chunky_get_incomplete(
        const chunkybar_t * prog,
        unsigned int *offset,
        unsigned int *len,
        const unsigned int max);

unsigned int chunky_get_nbytes_completed(
        const chunkybar_t * prog);

/**
 * @return 1 if we have this chunk, 0 otherwise */
int chunky_have(
        const chunkybar_t * prog,
        const unsigned int offset,
        const unsigned int len);

void chunky_print_contents(
        const chunkybar_t * prog);

#endif /* CHUNKYBAR_H */
