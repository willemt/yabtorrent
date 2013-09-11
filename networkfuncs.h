int peer_connect(
        void* caller,
        void **udata,
        void **nethandle,
        const char *host, int port,
        int (*func_process_data) (void *caller,
                        void* nethandle,
                        const unsigned char* buf,
                        unsigned int len),
        void (*func_process_connection) (void *, void* nethandle, char *ip, int iplen),
        void (*func_connection_failed) (void *, void* nethandle)
        );

int peer_send(void* caller, void **udata,
                  void* nethandle,
                  const unsigned char *send_data, const int len);

int peer_disconnect(void* caller, void **udata, void* nethandle);

int peers_poll(void* caller, void **udata,
                   const int msec_timeout,
                   int (*func_process) (void *caller,
                                    void* nethandle,
                                    const unsigned char* buf,
                                    unsigned int len),
                   void (*func_process_connection) (void *,
                                                    void* nethandle,
                                                    char *ip,
                                                    int));

int peer_listen_open(void* caller, void **udata, const int port);
