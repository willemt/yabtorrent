int tracker_connect(
    void *udata,
    const char *host,
    const char *port,
    int *id
)
{

}

int tracker_send(
    void *udata,
    int id,
    const void *send,
    int len
)
{

}

int tracker_receive(
    void *udata,
    int id,
    void **recv,
    int *rlen
)
{

}

int tracker_disconnect(
    void *udata,
    int id
)
{

}

int peer_connect(
    void *udata,
    const char *host,
    const char *port,
    int *peerid
)
{

}

int peer_send(
    void *udata,
    int peerid,
    void *send_data,
    int len
)
{


}

int peer_receive(
    void *udata,
    int peerid,
    void **recv,
    int *len
)
{



}

int peer_disconnect(
    void *udata,
    int peerid
)
{


}
