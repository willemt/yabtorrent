
#include "linked_list_hashmap.h"
#include "linked_list_queue.h"
#include "cbuffer.h"

void *__clients = NULL;

typedef struct {
//    llqueue_t* inbox;
//    bitstream_t* inbox;
    void* inbox;
    void* bt;

    /* id that we use to identify the client */
    int id;
} client_t;

/*8
typedef struct {
    void* data;
    int len;
} msg_t;
*/

int func_send(
    void *udata,
    const void * peer,
    const void *send_data,
    const int len
)
{
    slot_t* s;
#if 0
    msg_t* m;

    m = malloc(sizeof(msg_t));

    m->data = malloc(len);
    memcpy(m->data,send_data,len);
    m->len = len;
#else
//    void* data;

//    data = malloc(len);
//    memcpy(data,send_data,len);
#endif

    cbuf_offer(pr->inbox, send_data, len);
//    llqueue_offer(s->inbox, m);
}

#if 0
static unsigned long __uint_hash(
    const void *e1
)
{
    const long i1 = (unsigned long) e1;

    assert(i1 >= 0);
    return i1;
}

static long __uint_compare(
    const void *e1,
    const void *e2
)
{
    const long i1 = (unsigned long) e1, i2 = (unsigned long) e2;

//      return !(*i1 == *i2); 
    return i1 - i2;
}

#endif

client_t* __get_client_from_id(int id)
{
    client_t* cli;

    cli = hashmap_get(__clients, &id);

    return cli;
}

static unsigned long __int_hash(
    const void *e1
)
{
    const int *i1 = e1;

    return *i1;
}

static long __int_compare(
    const void *e1,
    const void *e2
)
{
    const int *i1 = e1, *i2 = e2;

    return *i1 - *i2;
}


int func_recv_f(
    void *udata,
    void *peer,
    char *buf,
    int *len
)
{
    client_t* cli;

    cli = __get_client_from_id();

    memcpy(buf, cbuf_poll(me->inbox, len), *len);
    cbuf_poll_release(me->inbox, *len)
}

int func_disconnect_f(
    void *udata,
    void * peer,
    char *reason
)
{

}

int func_connect_f(
    void *bto,
    void *pc,
    void * peer
)
{


}


client_t* client_setup()
{
    client_t* cli;
    void *bt;
    config_t* cfg;

    cli = malloc(sizeof(client_t));

    /* message inbox */
    cli->inbox = cbuf_new(16);

    /* bittorrent client */
    cli->bt = bt = bt_client_new();
    cfg = bt_client_get_config(bt);
    bt_client_set_peer_id(bt, bt_generate_peer_id());

    /* create disk backend */
    {
        void* dc;

        dc = bt_diskmem_new();
        bt_diskmem_set_size(dc, 1000);
        bt_client_set_diskstorage(bt, bt_diskmem_get_blockrw(dc), NULL, dc);
    }


    /* put inside the hashmap */
    cli->id = hashmap_count(__clients);
    hashmap_put(__clients,&cli->id,cli);

    return cli;
}


void setup()
{
    __clients = hashmap_new(__int_hash, __int_compare, 11);

    void* a, *b;

    hashmap_iterator_t iter;

    hashmap_iterator(__clients, &iter);
    while (hashmap_iterator_has_next(__clients, &iter))
    {
        void* bt, *cfg;

        bt = hashmap_iterator_next(__clients, &iter);
        cfg = bt_client_get_config(bt);
        config_set(cfg, "npieces", "1");
        config_set(cfg, "piece_length", "10");
        config_set(cfg,"infohash", "00000000000000000000");
        bt_client_add_pieces(bt, "00000000000000000000", 1);
    }

    a = client_setup();
    b = client_setup();

    bt_client_step(a);
    bt_client_step(b);
}
