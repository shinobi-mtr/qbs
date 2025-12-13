#include "bytes.h"
#include "file.h"
#include "tcp.h"
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

#define TEST "this is a test bytes. blah, blah ... blah\n"
#define TEST2 TEST TEST
#define TEST3 TEST2 TEST2
#define TEST4 TEST3 TEST3
#define TEST5 TEST4 TEST4
#define TEST6 TEST5 TEST5

void test_copy_n() {
  unsigned char in[] = {TEST6};
  uint8_t out[2024] = {0};

  qbs_bytes_io_t r = qbs_bytes_reader(in, sizeof(TEST6));
  qbs_bytes_io_t w = qbs_bytes_reader(out, sizeof(out));

  uint64_t n = sizeof(TEST) - 1;
  qbs_io_respose_t rio = qbs_io_copy_n(&r.io, &w.io, n);

  assert(rio.err == qbs_io_err_null);
  assert(rio.n == n);
}

void test_copy() {
  unsigned char in[] = {TEST6};
  uint8_t out[2024] = {0};

  qbs_bytes_io_t r = qbs_bytes_reader(in, sizeof(TEST6));
  qbs_bytes_io_t w = qbs_bytes_reader(out, sizeof(out));
  qbs_io_respose_t rio = qbs_io_copy(&r.io, &w.io);

  assert(rio.err == qbs_io_err_null);
  assert(rio.n == sizeof(in));
}

void test_read_full() {
  unsigned char in[] = {TEST6};
  unsigned char out[1111] = {0};

  qbs_bytes_io_t r = qbs_bytes_reader(in, sizeof(TEST6));
  qbs_io_respose_t rio = qbs_io_read_full(&r.io, out, sizeof(out));

  assert(rio.err == qbs_io_err_null);
  assert(rio.n == sizeof(out));
}
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

void test_socket_listener() {
  qbs_tcp_listener_t l = qbs_tcp_listen("0.0.0.0", 8000);
  assert(l.err == 0);

  qbs_io_tcp_t c1 = qbs_tcp_accept(&l);
  qbs_io_tcp_t c2 = qbs_tcp_accept(&l);

  assert(c1.err == 0);
  assert(c2.err == 0);

  qbs_io_respose_t r = qbs_io_copy(&c1.io, &c2.io);
  assert(r.err == qbs_io_err_null);
}

int main() {
#define TEST_SOCK_LISTENER

#ifdef TEST_IO
  test_copy();
  test_copy_n();
  test_read_full();
#endif

#ifdef TEST_FILE_TO_SOCK
  test_from_socket_to_file();
#endif

#ifdef TEST_SOCK_TO_FILE
  test_from_file_to_socket();
#endif

#ifdef TEST_SOCK_LISTENER
  test_socket_listener();
#endif
  return 0;
}
