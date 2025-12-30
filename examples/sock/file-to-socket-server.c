#include <stdint.h>
#define QBS_IMPL

#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>

int main(void) {
  qbs_listener_t l = qbs_tcp_listen("localhost", 8080);
  assert(l.err == false);

  while (1) {
    qbs_sock_t s = qbs_tcp_accept(&l);
    assert(s.err == false);

    qbs_file_t f = qbs_file_open("./assets/testfile.text", O_RDONLY);
    assert(f.err == false);

    uint64_t r = qbs_io_copy(&f.io, &s.io);
    assert(r != 0);

    s.io.close(&s);
    f.io.close(&f);
  }
  return 0;
}
