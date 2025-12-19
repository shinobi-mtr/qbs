#include "../../bytes.h"
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

  qbs_bytes_io_t r = qbs_bytes_reader(in, sizeof(in));
  qbs_io_respose_t rio = qbs_io_read_full(&r.io, out, sizeof(out));

  assert(rio.err == qbs_io_err_null);
  assert(rio.n == sizeof(out));
}
