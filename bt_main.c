
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <poll.h>

//#include <unistd.h>     /* for sleep */

#include <fcntl.h>

#include <getopt.h>


#include "bt.h"
#include "bt_main.h"
#include "config.h"

#define PROGRAM_NAME "bt"
/*----------------------------------------------------------------------------*/

#if 0
int peer_recv_len(
    void **udata,
    const int peerid,
    char *buf,
    int *len
)
{
    net_t *net;

    sock_t *sock;

    int recvd = 0;

    net = *udata;
    sock = &net->peer_socks[peerid];
//    printf("recv: len:%d\n", *len);
    if (0 < (recvd = recv(sock->fd, buf, *len, MSG_WAITALL)))
    {
#if 0
        int ii;

        for (ii = 0; ii < recvd; ii++)
        {
            printf("%c,", buf[ii]);
        }
        printf("\n");
#endif
//        buf[recvd] = '\0';
        assert(recvd == *len);
    }
    else
    {
        perror("unable to receive from peer");
        exit(EXIT_FAILURE);
        return 0;
    }

    if (recvd == 0)
    {
        printf("peer has shutdown\n");
        perror("recv from socket:");
        assert(0);
        return 0;
    }

    return 1;
}
#endif


char __cfg_bound_iface[32];


#include <sys/time.h>

static void __log(
    void *udata,
    void *src,
    char *buf
)
{
    int fd = (unsigned long) udata;

    struct timeval tv;

    gettimeofday(&tv, NULL);
//    tv.tv_sec // seconds
//    tv.tv_usec // microseconds

    char stamp[32];

    sprintf(stamp, "%d,%0.2f,", (int) tv.tv_sec, (float) tv.tv_usec / 100000);
    write(fd, stamp, strlen(stamp));
    write(fd, buf, strlen(buf));
    printf("%s\n", buf);
}

void usage(
    int status
)
{
#if 0
    if (status != EXIT_SUCCESS)
        fprintf(stderr, ("Try `%s --help' for more information.\n"),
                PROGRAM_NAME);
    else
#endif
    {
        printf("\
Usage: %s [OPTION]... TORRENT_FILE\n", PROGRAM_NAME);
        fputs(("\
Download torrent indicated by TORRENT_FILE. \n\n\
Mandatory arguments to long options are mandatory for short options too. \n\
  -i, --using-interface        network interface to use \n\
  -e, --verify-download        check downloaded files and quit \n\
  -b                                                    \n "), stdout);
        exit(status);
    }
}

static struct option const long_opts[] = {
    {
     "archive", no_argument, NULL, 'a'},
//  {" backup ", optional_argument, NULL, 'b'},
    {
     "using-interface", required_argument, NULL, 'i'},
    {"verify-download", no_argument, NULL, 'e'},
    {"shutdown-when-complete", no_argument, NULL, 's'},
    {"pwp_listen_port", required_argument, NULL, 'p'},
    {
     NULL, 0, NULL, 0}
};

#if 1
int main(
    int argc,
    char **argv
)
{
    char c;

    int o_verify_download = 0;

    int o_shutdown_when_complete = 0;

    void *bt;

    bt = bt_client_new();

    // setlocale(LC_ALL, " ");
    //  atexit (close_stdin);

    while ((c = getopt_long(argc, argv, "esi:p:", long_opts, NULL)) != -1)
    {
        switch (c)
        {
        case 's':
            printf("shutdown when complete\n");
            o_shutdown_when_complete = 1;
            break;

        case 'p':
            printf("opt: %s\n", optarg);
            bt_client_set_opt(bt, "pwp_listen_port", optarg);
            break;

        case 'i':
            strcpy(__cfg_bound_iface, optarg);
            printf("interface: %s\n", optarg);
            break;

        case 'e':
            o_verify_download = 1;
            break;

        default:
            usage(EXIT_FAILURE);
        }
    }

//    printf("%d '%s'\n", optind, argv[optind]);


    int status;

    status = config_read(" yabtc ", " config ");
//    printf(" test % d % s \ n ", status, config_get(" value "));
    /* do configuration */
    char *str;

    if ((str = config_get(" max_peer_connections ")))
        bt_client_set_max_peer_connections(bt, atoi(str));
    if ((str = config_get(" select_timeout_msec ")))
        bt_client_set_select_timeout_msec(bt, atoi(str));
    if ((str = config_get(" max_cache_mem_mb ")))
        bt_client_set_max_cache_mem(bt, atoi(str));

    bt_client_set_logging(bt,
                          open("dump_log", O_CREAT | O_TRUNC | O_RDWR,
                               0666), __log);

    bt_client_set_opt_shutdown_when_completed(bt, o_shutdown_when_complete);
//    if (bt_client_set_func

    bt_client_set_net_funcs(bt, &netfuncs);
    bt_client_set_path(bt, ".");
    bt_client_set_peer_id(bt, bt_generate_peer_id());
//    bt_set_log_func(bt,);

    if (argc == optind)
    {
        printf("ERROR: Please specify torrent file\n");
        exit(EXIT_FAILURE);
    }
    else if (0 == bt_client_read_metainfo_file(bt, argv[optind]))
    {
        exit(EXIT_FAILURE);
    }

//    bt_client_set_download_destination_folder(bt, " downloads
    if (o_verify_download)
    {

    }
    else
    {
        while (0 == bt_client_connect_to_tracker(bt))
            sleep(1);
        bt_client_go(bt);
    }

    bt_client_release(bt);
//    bt_connect_to_tracker(bt, bt_get_nbytes_downloaded)

    return 1;
}
#endif
