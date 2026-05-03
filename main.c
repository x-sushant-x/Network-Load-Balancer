#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#define PORT 8080

int main(void) {
    struct sockaddr_in server_sockaddr_in;

    server_sockaddr_in.sin_family = AF_INET;

    /*
    * htonl() converts host byte order to network byte order. l mean unsigned
    * long integer.
    *f
    * INADDR_ANY bind socket to all available network interfaces of
    * a mahcine. eg WiFi, Ethernet
    */
    server_sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY);

    // htons converts host byte order to network byte order for unsigned short
    // integer.
    server_sockaddr_in.sin_port = htons(PORT);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("unable to create socket: %s", strerror(errno));
    }

    int res = bind(socket_fd, (struct sockaddr *)&server_sockaddr_in, sizeof(server_sockaddr_in));
    if (res == -1) {
        printf("unable to bind socket: %s", strerror(errno));
    }

    listen(socket_fd, 5);

    return 0;
}
