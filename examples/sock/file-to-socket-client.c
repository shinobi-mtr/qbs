#include <stdint.h>
#define QBS_IMPL

#include <assert.h>
#include <fcntl.h>

#include "../../qbs.h"

int main(void) {
  qbs_file_t f = qbs_file_open("./assets/testfile.text", O_RDONLY);
  assert(f.err == false);

  qbs_sock_t s = qbs_tcp_dail("127.0.0.1", 8080);
  assert(s.err == false);

  uint64_t r = qbs_io_copy(&f.io, &s.io);
  assert(r != 0);

  s.io.close(&s);
  f.io.close(&f);

  return 0;
}
