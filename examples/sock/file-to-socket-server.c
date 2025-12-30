#include <stdint.h>
#define QBS_IMPL

#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>

int main(void) {
  qbs_listener_t l = {};
  assert(qbs_tcp_listen(&l, "localhost", 8080) == true);

  while (1) {
    qbs_sock_t s = {};
    qbs_file_t f = {};

    assert(qbs_tcp_accept(&s, &l) == true);
    assert(qbs_file_open(&f, "./assets/testfile.text", O_RDONLY) == true);
    assert(qbs_io_copy(&f.io, &s.io) != 0);

    s.io.close(&s);
    f.io.close(&f);
  }
  return 0;
}
