int peer_connect(void **udata, const char *host, int port, void **nethandle,
        void (*func_process_connection) (void *, void* nethandle, char *ip, int iplen));

int peer_send (void **udata,
                  void* nethandle,
                  const unsigned char *send_data, const int len);

int peer_disconnect (void **udata, void* nethandle);

int peers_poll (void **udata,
                   const int msec_timeout,
                   int (*func_process) (void *caller,
                                    void* nethandle,
                                    const unsigned char* buf,
                                    unsigned int len),
                   void (*func_process_connection) (void *,
                                                    void* nethandle,
                                                    char *ip,
                                                    int), void *data);

int peer_listen_open (void **udata, const int port);

void* network_setup();
