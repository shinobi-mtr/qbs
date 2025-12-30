#include <stdint.h>
#define QBS_IMPL

#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>

int main(void) {
  qbs_listener_t l = {};
  assert(qbs_tcp_listen(&l, "localhost", 8080) == true);

  while (1) {
    qbs_sock_t s1 = {};
    qbs_sock_t s2 = {};

    assert(qbs_tcp_accept(&s1, &l) == true);
    assert(qbs_tcp_accept(&s2, &l) == true);
    assert(qbs_io_copy(&s1.io, &s2.io));

    s1.io.close(&s1);
    s2.io.close(&s2);
  }
  return 0;
}
