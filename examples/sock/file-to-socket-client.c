#define QBS_IMPL

#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>

int main(void) {
  qbs_file_t f = {};
  qbs_sock_t s = {};

  assert(qbs_file_open(&f, "./assets/testfile.text", O_RDONLY) == true);
  assert(qbs_tcp_dial(&s, "127.0.0.1", 8080) == true);
  assert(qbs_io_copy(&f.io, &s.io) != 0);

  s.io.close(&s);
  f.io.close(&f);

  return 0;
}
