
typedef struct
{
    void *first_block;
    int max;
} raprogress_t;

void raprogress_get_incomplete(
    raprogress_t * prog,
    int *offset,
    int *len,
    const int max
);

void *raprogress_init(
    const int max
);

int raprogress_get_num_blocks(
    raprogress_t * prog
);

void raprogress_mark_complete(
    raprogress_t * prog,
    int offset,
    int len
);

bool raprogress_is_complete(
    raprogress_t * prog
);

void raprogress_get_incomplete(
    raprogress_t * prog,
    int *offset,
    int *len,
    const int max
);
