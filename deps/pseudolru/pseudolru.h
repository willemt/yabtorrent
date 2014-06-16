#ifndef PSEUDOLRU_H
#define PSEUDOLRU_H


typedef struct
{
    void *root;
    int (
    *cmp
    )   (
    const void *,
    const void *
    );
    int count;
} pseudolru_t;

pseudolru_t *pseudolru_new(
    int (*cmp) (const void *,
                const void *)
);

void pseudolru_free(
    pseudolru_t * me
);

int pseudolru_is_empty(
    pseudolru_t * me
);


/**
 * Get this item referred to by key.
 * Set it as root */
void *pseudolru_get(
    pseudolru_t * me,
    const void *key
);

void *pseudolru_remove(
    pseudolru_t * me,
    const void *key
);

void *pseudolru_pop_lru(
    pseudolru_t * dt
);

int pseudolru_count(
    pseudolru_t * me
);

void *pseudolru_peek(
    pseudolru_t * me
);

void pseudolru_put(
    pseudolru_t * me,
    void *key,
    void *value
);

#endif /* PSEUDOLRU_H */
