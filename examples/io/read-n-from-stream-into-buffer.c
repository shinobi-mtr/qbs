#define QBS_IMPL

#include "../../qbs.h"
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

#define A "A"
#define A4 A A A A
#define A16 A4 A4 A4 A4
#define A64 A16 A16 A16 A16
#define A254 A64 A64 A64 A64
#define A1024 A254 A254 A254 A254
#define A2048 A1024 A1024 A1024 A1024

int main(void) {
  unsigned char in[] = {A2048};
  uint8_t out[2024] = {0};

  qbs_bytes_t r = qbs_bytes_reader(in, sizeof(in));
  qbs_bytes_t w = qbs_bytes_writer(out, sizeof(out));

  uint64_t n = qbs_io_copy_n(&r.io, &w.io, 1024);

  assert(n == 1024);

  return 0;
}
