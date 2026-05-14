#include "../types.h"
#include "../lb/utils.h"

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>

/*
 * <sys/event.h> is macos specific. On linux we use epoll for event poll implementation.
 * But as I'm on macos. We are using kqueue which is macos alternative for epoll.
 * Later will port this event poll implementation to epoll on Linux.
 */

#include <sys/event.h>


/*
 * Kernel manages 2 queues.
 * 1. SYN Queue -> This store half open connections that are not fully established yet.
 * 2. Accept Queue -> This stored the connections that are completely established.
 */
#define BACKLOG_SIZE 4096

#define MSG_MAX_SIZE 1024
#define MAX_EVENTS 1024

/*
 * This function enables non-blocking mode on any file descriptor.
 * Syscalls like read(), write(), accept() are blocking by default. If there is no data program waits (get stucked).
 * In non-blocking mode these functions returns instantly instead of waiting.
 */
static int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    // flags | O_NONBLOCK uses a bitwise OR operator which keeps existing flags and add the new.
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void enable_write_event(int kq, connection_t* conn) {
    struct kevent ev;

    EV_SET(
        &ev,
        conn->fd,
        EVFILT_WRITE,
        EV_ADD,
        0,
        0,
        conn
    );

    kevent(kq, &ev, 1, NULL, 0, NULL);
}

static void disable_write_event(int kq, connection_t *conn) {
    struct kevent ev;

    EV_SET(
        &ev,
        conn->fd,
        EVFILT_WRITE,
        EV_DELETE,
        0,
        0,
        NULL
    );

    kevent(kq, &ev, 1, NULL, 0, NULL);
}


void start(int port) {
    BackendPool* backends = load_backend();
    printf("Loaded %d backend servers.\n", backends -> count);

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
    server_sockaddr_in.sin_port = htons(port);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("unable to create socket");
        exit(-1);
    }

    if (set_non_blocking(server_fd) == -1) {
        perror("unable to enable non blocking mode on socket");
        exit(-1);
    }

    if (bind(server_fd, (struct sockaddr *)&server_sockaddr_in, sizeof(server_sockaddr_in)) == -1) {
        perror("unable to bind socket");
        exit(-1);
    }

    printf("Proxy listening on port: %d\n", port);

    /*
     * When a server starts listening for connections, the kernel maintains internal queues to manage incoming TCP requests
     * before the application officially "accepts" them with the accept() call.
     */
    if (listen(server_fd, BACKLOG_SIZE) == -1) {
        perror("unable to listen on socket");
        exit(-1);
    }

    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        exit(1);
    }

    /* This struct represent an event that we want to watch. */
    struct kevent ev;

    /*
     * EV_SET is a helper macro that fils kevent struct.
     *
     * &ev -> struct to fill.
     * socket_fd -> file descriptor that we want to watch.
     * EVFILT_READ -> kind of event.
     * ED_ADD -> Adding event. Will use EV_DEL when removing event from kqueue.
     *
     */
    EV_SET(&ev, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

    /*
     * kevent syscall register, monitor and receive events from kernel.
     *
     * kq -> kqueue instance.
     * &ev -> changes to do. can also pass arr of kevent.
     * 1 -> number of changes.
     * NULL -> telling kernel not interesting in receiving back events right now. because we are just regestering yet not waiting. it is called event list.
     * 0 -> size of event list. 0 because we passed NULL.
     */

    if (kevent(kq, &ev, 1, NULL, 0, NULL) == -1) {
        perror("unable to register kevent");
        exit(1);
    }

    struct kevent events[MAX_EVENTS];


    while (1) {
        /*
         * kevent comes with 2 modes:
         * 1. kevent(kq, &ev, 1, NULL, 0, NULL); -> Add, remove or update events.
         * 2. kevent(kq, NULL, 0, events, MAX_EVENTS, NULL); -> wait for events.
         *
         * nev is total number of ready events that can be processed. this will also fill events array.
         */
        int nev = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);

        if (nev == -1) {
            perror("kevent wait");
            exit(1);
        }

        /* Processing each ready event using it's file descriptor. */
        for (int i = 0; i < nev; i++) {
            int fd = (int)events[i].ident;

            /* New incomming connections. */
            if (fd == server_fd) {
                while(1) {
                    struct sockaddr_in client_addr;
                    socklen_t len = sizeof(client_addr);

                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
                    if (client_fd == -1) {
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

                    if (set_non_blocking(client_fd) == -1) {
                        perror("error while enabling non blocking mode in conn_fd");
                        close(client_fd);
                        continue;
                    }

                    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
                    set_non_blocking(backend_fd);

                    struct sockaddr_in backend_addr;
                    backend_addr.sin_family = AF_INET;
                    backend_addr.sin_port = htons(9000);

                    inet_pton(
                        AF_INET,
                        "127.0.0.1",
                        &backend_addr.sin_addr
                    );

                    int ret = connect(backend_fd, (struct sockaddr*) &backend_addr, sizeof(backend_addr));
                    if (ret == -1) {
                        if (errno == EINPROGRESS) {
                            printf("connecting to backend...");
                        } else {
                            perror("unable to connect to backend.");
                            close(client_fd);
                            continue;
                        }
                    }

                    connection_t* client_conn = malloc(sizeof(connection_t));
                    connection_t* backend_conn = malloc(sizeof(connection_t));

                    memset(client_conn, 0, sizeof(connection_t));
                    memset(backend_conn, 0, sizeof(connection_t));

                    client_conn -> fd = client_fd;
                    backend_conn -> fd = backend_fd;

                    /*
                     * Here we created a two-way tunnel. peer of client socket is backend socket and peer of backend socket is client socket.
                     */

                    client_conn -> peer = backend_conn;
                    backend_conn -> peer = client_conn;

                    struct kevent client_ev;

                    EV_SET(
                        &client_ev,
                        client_fd,
                        EVFILT_READ,
                        EV_ADD,
                        0,
                        0,
                        client_conn
                    );

                    kevent(kq, &client_ev, 1, NULL, 0, NULL);

                    struct kevent backend_ev;

                    EV_SET(
                        &backend_ev,
                        backend_fd,
                        EVFILT_READ,
                        EV_ADD,
                        0,
                        0,
                        backend_conn
                    );

                    kevent(kq, &backend_ev, 1, NULL, 0, NULL);
                }
            }
            /*
             * Read Events
             */
            else if (events[i].filter == EVFILT_READ) {
                connection_t *conn = (connection_t *) events[i].udata;
                connection_t *peer = conn -> peer;

                char temp[MSG_MAX_SIZE];

                /*
                 * Following code read data from current socket and than copies data to peer buffer.
                 * This means that client socket data is copied to backend socket buffer.
                 *
                 * Same thing happen in reverse when we read some data from backend socket and copy to client buffer.
                 */
                ssize_t n = read(conn -> fd, &temp, sizeof(temp));

                if (n <= 0) {
                    close(conn -> fd);
                    continue;
                }

                memcpy(peer -> buffer, temp, n);
                peer -> buffer_len = n;
                peer -> buffer_sent = 0;

                /*
                 * Now we unabled kqueue write event for backend socket. This means that we are asking macOS to
                 * notify us when backend socket can send data.
                 *
                 * Same thing happen in reverse when we read some data from backend socket and copy to client buffer.
                 */
                enable_write_event(kq, peer);
            }
            /*
             * Write Events
             */
            else if (events[i].filter == EVFILT_WRITE) {
                connection_t *conn = (connection_t *) events[i].udata;

                while(conn -> buffer_sent < conn -> buffer_len) {
                    size_t remaining = conn -> buffer_len - conn -> buffer_sent;

                    ssize_t n = write(conn -> fd, conn -> buffer + conn -> buffer_sent, remaining);

                    if (n > 0) {
                        conn->buffer_sent += n;
                    } else if (n == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }

                        close(conn -> fd);
                        break;
                    }
                }

                if (conn->buffer_sent == conn->buffer_len) {
                    conn->buffer_len = 0;
                    conn->buffer_sent = 0;

                    disable_write_event(kq, conn);
                }
            }
        }
    }
}
