#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define PORT 8080
#define BACKLOG_SIZE 128
#define MSG_MAX_SIZE 1024

/*
 * This function enabled non-blocking mode on any file descriptor.
 * Syscalls like read(), write(), accept() are blocking by default. If there is no data program waits (get stucked).
 * In non-blocking mode these functions returns instantly instead of waiting.
 */
int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    // flags | O_NONBLOCK uses a bitwise OR operator which keeps existing flags and add the new.
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void) {
    struct sockaddr_in server_sockaddr_in;

    server_sockaddr_in.sin_family = AF_INET;

    /*
    * htonl() converts host byte order to network byte order. l mean unsigned
    * long integer.
    *
    * INADDR_ANY bind socket to all available network interfaces of
    * a mahcine. eg WiFi, Ethernet
    */
    server_sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    /*htons converts host byte order to network byte order for unsigned short integer. */
    server_sockaddr_in.sin_port = htons(PORT);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("unable to create socket: %s", strerror(errno));
        exit(-1);
    }

    if (set_non_blocking(socket_fd) == -1) {
        printf("unable to enable non blocking mode on socket: %s", strerror(errno));
        exit(-1);
    }

    int res = bind(socket_fd, (struct sockaddr *)&server_sockaddr_in, sizeof(server_sockaddr_in));
    if (res == -1) {
        printf("unable to bind socket: %s", strerror(errno));
        exit(-1);
    }

    printf("listening on port: %d\n", PORT);

    /*
     * When a server starts listening for connections, the kernel maintains internal queues to manage incoming TCP requests
     * before the application officially "accepts" them with the accept() call.
     */
    res = listen(socket_fd, BACKLOG_SIZE);
    if (res == -1) {
        printf("unable to listen on socket: %s", strerror(errno));
        exit(-1);
    }

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    while (1) {
        int conn_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &len);
        printf("connection accepted\n");

        char buffer[MSG_MAX_SIZE];

        while (1) {
            memset(buffer, 0, sizeof(buffer));

            int n = read(conn_fd, buffer, sizeof(buffer) - 1);
            if (n < 0) {
                printf("unable to read from client: %s", strerror(errno));
                exit(-1);
            }

            buffer[n] = '\0';
        }
    }

    return 0;
}
