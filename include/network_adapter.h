#ifndef NETWORK_ADAPTER_FUNCS_H
#define NETWORK_ADAPTER_FUNCS_H


/**
 * Note: see functions documentation within network_adapter.h
 */

int peer_connect(
        void* caller,
        void **udata,
        void **nethandle,
        const char *host, int port,
        int (*func_process_data) (void *caller,
                        void* nethandle,
                        const char* buf,
                        unsigned int len),
        int (*func_process_connection) (void *, void* nethandle, char *ip, int port),
        void (*func_connection_failed) (void *, void* nethandle)
        );

int peer_send(void* caller, void **udata,
                  void* nethandle,
                  const char *send_data, const int len);

int peer_disconnect(void* caller, void **udata, void* nethandle);

int peer_listen(void* caller,
        void **nethandle,
        int port,
        int (*func_process_data) (void *caller,
                        void* nethandle,
                        const char* buf,
                        unsigned int len),
        int (*func_process_connection) (void *, void* nethandle, char *ip, int iplen),
        void (*func_connection_failed) (void *, void* nethandle));


#endif /* NETWORK_ADAPTER_FUNCS_H */
