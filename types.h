#include <unistd.h>

typedef struct {
  int fd;
  const char *response; // data waiting to be sent
  size_t response_len;  // total bytes to send
  size_t response_sent; // total bytes already written
} connection_t;
