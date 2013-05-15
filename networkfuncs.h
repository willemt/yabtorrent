int peer_connect (void **udata,
                     const char *host, const char *port, int *peerid);

int peer_send (void **udata,
                  const int peerid,
                  const unsigned char *send_data, const int len);

int peer_recv_len (void **udata, int peerid, char *recv, int *len);

int peer_disconnect (void **udata, int peerid);

int peers_poll (void **udata,
                   const int msec_timeout,
                   int (*func_process) (void *,
                                        int),
                   void (*func_process_connection) (void *,
                                                    int netid,
                                                    char *ip,
                                                    int), void *data);

int peer_listen_open (void **udata, const int port);
