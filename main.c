#include "streams.h"
#include <assert.h>

void test_from_file_to_socket() {
  qbs_file_t fxt = {
      .filename = "test.txt",
      .mode = O_RDWR | O_CREAT,
  };
  qbs_io_t file = {0};

  qbs_tcp_t ttx = {
      .ip = "0.0.0.0",
      .port = 8888,
  };
  qbs_io_t tcp = {0};

  assert(qbs_file_open(&file, &fxt) == 0);
  assert(qbs_tcp_connect(&tcp, &ttx) == 0);

  qbs_io_respose_t cr = qbs_io_copy(&tcp, &file);
  assert(cr.err == qbs_io_err_null);
}

void test_from_socket_to_file() {
  qbs_file_t fxt = {
      .filename = "test.txt",
      .mode = O_RDWR | O_CREAT,
  };
  qbs_io_t file = {0};

  qbs_tcp_t ttx = {
      .ip = "0.0.0.0",
      .port = 8888,
  };
  qbs_io_t tcp = {0};

  assert(qbs_file_open(&file, &fxt) == 0);
  assert(qbs_tcp_connect(&tcp, &ttx) == 0);

  qbs_io_respose_t cr = qbs_io_copy(&file, &tcp);
  assert(cr.err == qbs_io_err_null);
}

int main() {
  test_from_socket_to_file();
  test_from_file_to_socket();
  return 1;
}
