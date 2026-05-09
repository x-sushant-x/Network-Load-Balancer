#include "net/server.h"

#define PORT 8080

int main(void) {
    /*
     * start() will setup a TCP server on given port to accept client requests.
     * server will run in non-blocking mode.
     * that means no new threads will be created for each request.
     * instead an event poll is implemented inside start() which handle multiple requests based on events and in single thread.
     */
    start(PORT);
}
