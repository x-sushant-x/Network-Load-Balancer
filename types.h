#ifndef TYPES_h
#define TYPES_h

#include "net/server.h"
#include <unistd.h>

typedef struct connection {
  int fd;

  /*
   * peer is the connection between client -> backend and backend -> client.
   * two sockets will be created for every connection.
   * 1. client socket (our load balancer socket)
   * 2. backend socket
   *
   * whatever client reads it will write to backend socket. and whatever backend
   * socket response will be written to client socket.
   */
  struct connection *peer;

  char buffer[8192];

  size_t buffer_len;  // total bytes to send
  size_t buffer_sent; // total bytes already written
} connection_t;

typedef struct Backend {
  char *IP;
  int port;
  int health_check_port;
  int is_healthy;
} Backend;

typedef struct BackendPool {
  Backend *backends;
  int count;
  int current;
} BackendPool;

#endif
