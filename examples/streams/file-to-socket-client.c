#include <assert.h>
#include <fcntl.h>

#include "../../io.h"

#include "../../file.h"
#include "../../tcp.h"

int main(void) {
  qbs_io_file_t f = qbs_file_open("./assets/testfile.text", O_RDONLY);
  assert(f.err == 0);

  qbs_io_tcp_t s = qbs_tcp_dail("127.0.0.1", 8080);
  assert(s.err == 0);

  qbs_io_respose_t r = qbs_io_copy(&f.io, &s.io);
  assert(r.err == qbs_io_err_null);

  s.io.close(&s.ctx);
  f.io.close(&f.ctx);

  return 0;
}
