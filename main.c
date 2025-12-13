#include "streams.h"
#include <assert.h>
#include <fcntl.h>

void test_from_file_to_socket() {
  qbs_io_file_t f = qbs_file_open("test.txt", O_RDWR | O_CREAT);
  qbs_io_tcp_t t = qbs_tcp_dail("0.0.0.0", 8889);

  assert(f.err == 0);
  assert(t.err == 0);

  qbs_io_respose_t cr = qbs_io_copy(&t.io, &f.io);
  assert(cr.err == qbs_io_err_null);

  f.io.close(&f.ctx);
  t.io.close(&t.ctx);
}

void test_from_socket_to_file() {
  qbs_io_file_t f = qbs_file_open("test.txt", O_RDWR | O_CREAT);
  qbs_io_tcp_t t = qbs_tcp_dail("0.0.0.0", 8888);

  assert(f.err == 0);
  assert(t.err == 0);

  qbs_io_respose_t cr = qbs_io_copy(&f.io, &t.io);
  assert(cr.err == qbs_io_err_null);

  f.io.close(&f.ctx);
  t.io.close(&t.ctx);
}

int main() {
  test_from_socket_to_file();
  test_from_file_to_socket();
  return 1;
}
