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
  unsigned char out[1024] = {0};

  qbs_bytes_t r = {};
  assert(qbs_bytes_reader(&r, in, sizeof(in)) == true);
  assert(qbs_io_read_full(&r.io, out, sizeof(out)) == sizeof(out));
}
