
#if 0
typedef int (
    *func_write_block_f
)   (
    void *udata,
    void *caller,
    const bt_block_t * blk,
    const void *blkdata
);

typedef void *(
    *func_read_block_f
)    (
    void *udata,
    void *caller,
    const bt_block_t * blk
);

typedef struct
{
    func_write_block_f write_block;

    func_read_block_f read_block;

    /*  release this block from the holder of it */
//    func_giveup_block_f giveup_block;
} bt_blockrw_i;
#endif
