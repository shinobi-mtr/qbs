#include <stdint.h>
#define QBS_IMPL

#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>

int main(void) {
  const char route[] = "/";
  const char header[] = {"Host: localhost:8080\r\n"
                         "Accept: */*\r\n"
                         "Content-Length: 156\r\n"
                         "Content-Type: application/x-www-form-urlencoded\r\n"};
  qbs_file_t f = {};
  assert(qbs_file_open(&f, "./assets/testfile.text", O_RDONLY) == false);

  qbs_sock_t s = {};
  assert(qbs_http_post(&s, "127.0.0.1", 8080, route, sizeof(route) - 1, header, sizeof(header) - 1, &f.io) == false);

  uint8_t buff[2024] = {0};
  qbs_bytes_t b = {};

  assert(qbs_bytes_writer(&b, buff, 2024));

  uint64_t r = qbs_io_copy(&s.io, &b.io);
  assert(r != 0);

  return 0;
}
