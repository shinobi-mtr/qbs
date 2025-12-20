#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>

int main(void) {
  qbs_tcp_listener_t l = qbs_tcp_listen("localhost", 8080);
  assert(l.err == 0);

  while (1) {
    qbs_io_tcp_t s = qbs_tcp_accept(&l);
    assert(s.err == 0);

    qbs_io_file_t f = qbs_file_open("./assets/testfile.text", O_RDONLY);
    assert(f.err == 0);

    qbs_io_respose_t r = qbs_io_copy(&f.io, &s.io);
    assert(r.err == qbs_io_err_null);

    s.io.close(&s.ctx);
    f.io.close(&f.ctx);
  }
  return 0;
}
