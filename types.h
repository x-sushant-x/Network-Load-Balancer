#include "net/server.h"
#include <unistd.h>

typedef struct connection {
  int fd;

  struct connection *peer;

  char buffer[8192];

  size_t buffer_len;  // total bytes to send
  size_t buffer_sent; // total bytes already written
} connection_t;
