#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include <sys/event.h>

#define PORT 8080


#define BACKLOG_SIZE 128
#define MSG_MAX_SIZE 1024
#define MAX_EVENTS 1024

/*
 * This function enables non-blocking mode on any file descriptor.
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

    /* htons converts host byte order to network byte order for unsigned short integer. */
    server_sockaddr_in.sin_port = htons(PORT);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("unable to create socket");
        exit(-1);
    }

    if (set_non_blocking(socket_fd) == -1) {
        perror("unable to enable non blocking mode on socket");
        exit(-1);
    }

    if (bind(socket_fd, (struct sockaddr *)&server_sockaddr_in, sizeof(server_sockaddr_in)) == -1) {
        perror("unable to bind socket");
        exit(-1);
    }

    printf("listening on port: %d\n", PORT);

    /*
     * When a server starts listening for connections, the kernel maintains internal queues to manage incoming TCP requests
     * before the application officially "accepts" them with the accept() call.
     */
    if (listen(socket_fd, BACKLOG_SIZE) == -1) {
        perror("unable to listen on socket");
        exit(-1);
    }

    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        exit(1);
    }

    struct kevent ev;


    EV_SET(&ev, socket_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);



    if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
        perror("unable to register kevent");
        exit(1);
    }

    struct kevent events[MAX_EVENTS];


    while (1) {


        int nev = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);

        if (nev == -1) {
            perror("kevent wait");
            exit(1);
        }

        /* Processing each ready event using it's file descriptor. */
        for (int i = 0; i < nev; i++) {
            int fd = (int)events[i].ident;

            /* Received file descriptor is socket_fd. We will not accept incoming connections. */
            if (fd == socket_fd) {
                while(1) {
                    struct sockaddr_in client_addr;
                    socklen_t len = sizeof(client_addr);

                    int conn_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &len);
                    if (conn_fd == -1) {
                        /*
                         * These errors are occured when there are no more connections to accept.
                         */
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        } else {
                            perror("connection accept error");
                            break;
                        }
                    }

                    if (set_non_blocking(conn_fd) == -1) {
                        perror("error while enabling non blocking mode in conn_fd");
                        close(conn_fd);
                        continue;
                    }

                    struct kevent client_ev;
                    EV_SET(&client_ev, conn_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    kevent(kq, &client_ev, 1, NULL, 0, NULL);

                    printf("New connection: %d\n", conn_fd);
                }
            } else {
                char buffer[MSG_MAX_SIZE];

                int n = read(fd, buffer, sizeof(buffer) - 1);

                if (n <= 0) {
                    if (n == 0) {
                        printf("client disconnected: %d\n", fd);
                    } else {
                        perror("client read error");
                    }

                    close(fd);

                    struct kevent ev_del;
                    EV_SET(&ev_del, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    kevent(kq, &ev_del, 1, NULL, 0, NULL);
                } else {
                    buffer[n] = '\0';

                    printf("received from %d: %s\n", fd, buffer);

                    write(fd, buffer, n);
                }
            }
        }
    }

    return 0;
}
