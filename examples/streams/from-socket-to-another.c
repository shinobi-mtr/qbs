#include "../../tcp.h"
#include <assert.h>
#include <fcntl.h>

int main(void) {
  qbs_tcp_listener_t l = qbs_tcp_listen("localhost", 8080);
  assert(l.err == 0);

  while (1) {
    qbs_io_tcp_t s1 = qbs_tcp_accept(&l);
    assert(s1.err == 0);

    qbs_io_tcp_t s2 = qbs_tcp_accept(&l);
    assert(s2.err == 0);

    qbs_io_respose_t r = qbs_io_copy(&s1.io, &s2.io);
    assert(r.err == qbs_io_err_null);

    s1.io.close(&s1.ctx);
    s2.io.close(&s2.ctx);
  }
  return 0;
}
