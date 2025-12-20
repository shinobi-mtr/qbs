#include <stdint.h>
#include <stdio.h>
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
  qbs_file_t f = qbs_file_open("./assets/testfile.text", O_RDONLY);
  assert(f.err == 0);

  qbs_sock_t s = qbs_http_post("127.0.0.1", 80, route, sizeof(route) - 1, header, sizeof(header) - 1, &f.io);
  assert(s.err == 0);

  uint8_t buff[2024] = {0};
  qbs_bytes_t b = qbs_bytes_writer(buff, 2024);

  qbs_result_t r = qbs_io_copy(&s.io, &b.io);
  assert(r.err == 0);

  fprintf(stdout, "%s", buff);
  return 0;
}
