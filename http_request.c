
/*
 * 
 *
 *
 *
 *
 *
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <fcntl.h>

char *strndup(
    const char *s,
    size_t n
);

// GET /index.html HTTP/1.1
// Host: www.example.com
static char *__build_request(
    const char *url
)
{
    char *request;

    asprintf(&request,
             "GET %s http/1.0\r\n"
             "User-Agent: htmlclient\r\n" "\r\n\r\n", url);

    return request;
}

static char *__recv(
    const int sock
)
{
#define BUF_SIZE 32
    char buf[BUF_SIZE + 1];

    char *out = NULL;

    int recvd = 0;

    while ((recvd = recv(sock, buf, BUF_SIZE, 0)) > 0)
    {
        buf[recvd] = '\0';
        if (!out)
        {
            out = strndup(buf, recvd);
        }
        else
        {
            char *tmp = NULL;

            asprintf(&tmp, "%s%s", out, buf);
            assert(tmp);
            free(out);
            out = tmp;
        }
    }

    return out;
}

/**
 * @returns nonzero on success */
static int __send(
    int sock,
    const char *data
)
{
    int bytes;

    while ((bytes = write(sock, data, strlen(data))))
    {
        data += bytes;
    }

    if (bytes < 0)
    {
        perror("writing to socket");
        return 0;
//        exit(EXIT_FAILURE);
    }

    return 1;
}

//char *http_get_url_object(
char *http_get_response_from_request(
    const char *host,
    const char *port,
    const char *http_request
)
{
    int sock, err;

    char *response;

    //char *http_request, *response;

    struct addrinfo hints;

    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

//"192.168.10.231"

    err = getaddrinfo(host, port, &hints, &result);

    if (err != 0)
    {
        fprintf(stderr, "getaddrinfo: %s (%s,%s)\n",
                gai_strerror(err), host, port);
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (sock == -1)
            continue;

        assert(rp);
        assert(rp->ai_addr);
//        struct sockaddr_in    *sock_addr = rp->ai_addr;
//        printf("%s\n", inet_ntoa(sock_addr->sin_addr));

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) >= 0)
        {
            break;
        }

        close(sock);
    }

    if (rp == NULL)
    {
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

//    http_request = __build_request(url);
//    printf("request: %s\n", http_request);

    if (!__send(sock, http_request))
    {
        printf("failed to send data\n");
        return NULL;
    }

    response = __recv(sock);

    return response;
}

/*--------------------------------------------------------------79-characters-*/
