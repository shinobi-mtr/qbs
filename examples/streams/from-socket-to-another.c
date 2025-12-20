#define QBS_IMPL

#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>

int main(void) {
  qbs_listener_t l = qbs_tcp_listen("localhost", 8080);
  assert(l.err == 0);

  while (1) {
    qbs_sock_t s1 = qbs_tcp_accept(&l);
    assert(s1.err == 0);

    qbs_sock_t s2 = qbs_tcp_accept(&l);
    assert(s2.err == 0);

    qbs_result_t r = qbs_io_copy(&s1.io, &s2.io);
    assert(r.err == qbs_io_err_null);

    s1.io.close(&s1);
    s2.io.close(&s2);
  }
  return 0;
}
