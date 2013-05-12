
#include "linked_list_hashmap.h"
#include "linked_list_queue.h"
#include "cbuffer.h"

typedef struct {
//    llqueue_t* inbox;
//    bitstream_t* inbox;
    void* inbox;
} slot_t;

typedef struct {
    void* data;
    int len;
} msg_t;

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


int func_recv_f(
    void *udata,
    void * peer,
    char *buf,
    int *len
)
{
    slot_t* me;
    //void* data;
    //data = malloc(len);

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


void* client_setup()
{
    void *bt;
    config_t* cfg;

    bt = bt_client_new();
    cfg = bt_client_get_config(bt);
    bt_client_set_peer_id(bt, bt_generate_peer_id());

    return bt;
}

void setup()
{
    bt_client_step(bt);
}
