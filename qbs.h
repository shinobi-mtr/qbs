#ifndef QBS_H_
#define QBS_H_

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef QBSDEF
#define QBSDEF static inline
#endif

typedef enum {
  qbs_io_err_eof = -1,
  qbs_io_err_null = 0,
  qbs_io_err_unexpected_eof = 1,
  qbs_io_err_no_progress = 2,
  qbs_io_err_short_buffer = 3,
  qbs_io_err_long_buffer = 4,
  qbs_io_err_invalid_method = 5,
} qbs_io_err;

typedef struct {
  int64_t err;
  uint64_t n;
} qbs_io_respose_t;

typedef qbs_io_respose_t (*qbs_io_read)(void *ctx, uint8_t *bytes, uint64_t size);
typedef qbs_io_respose_t (*qbs_io_write)(void *ctx, uint8_t *bytes, uint64_t size);
typedef uint16_t (*qbs_io_close)(void *ctx);

typedef struct {
  qbs_io_read read;
  qbs_io_write write;
  qbs_io_close close;
  uint8_t ctx[0];
} qbs_io_t;

typedef struct {
  uint64_t limit;
  uint64_t done;
  qbs_io_t *r;
  bool is_completed;
} qbs_limit_ctx;

typedef struct {
  qbs_io_t io;
  qbs_limit_ctx ctx;
} qbs_io_limit_t;

typedef struct {
  const char *filename;
  const int mode;
  int fd;
} qbs_file_ctx_t;

typedef struct {
  qbs_io_t io;
  qbs_file_ctx_t ctx;
  int err;
} qbs_io_file_t;

typedef struct {
  const char *address;
  const uint16_t port;
  int sock;
} qbs_tcp_ctx_t;

typedef struct {
  qbs_io_t io;
  qbs_tcp_ctx_t ctx;
  int err;
} qbs_io_tcp_t;

typedef struct {
  int sock;
  struct sockaddr_in address;
  int err;
} qbs_tcp_listener_t;

typedef struct {
  uint64_t offset;
  const uint64_t capacity;
  uint8_t *buffer;
  bool is_completed;
} qbs_bytes_ctx_t;

typedef struct {
  qbs_io_t io;
  qbs_bytes_ctx_t ctx;
} qbs_bytes_io_t;

QBSDEF const uint8_t *qbs_io_error_to_string(int16_t code);

// IO Functions
QBSDEF qbs_io_respose_t qbs_io_copy(qbs_io_t *src, qbs_io_t *dst);
QBSDEF qbs_io_respose_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz);
QBSDEF qbs_io_respose_t qbs_io_copy_n(qbs_io_t *src, qbs_io_t *dst, uint64_t n);
QBSDEF qbs_io_respose_t qbs_io_read_at_least(qbs_io_t *r, uint8_t *b, uint64_t sz, uint64_t min);
QBSDEF qbs_io_respose_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz);

// Reader/Writer creation
QBSDEF qbs_io_limit_t qbs_io_add_limit(qbs_io_t *r, uint64_t limit);
QBSDEF qbs_io_file_t qbs_file_open(char *filename, int mode);
QBSDEF qbs_tcp_listener_t qbs_tcp_listen(char *address, uint16_t port);
QBSDEF qbs_io_tcp_t qbs_tcp_accept(qbs_tcp_listener_t *l);
QBSDEF qbs_io_tcp_t qbs_tcp_dail(char *address, uint16_t port);
QBSDEF qbs_bytes_io_t qbs_bytes_reader(uint8_t *buffer, uint64_t size);
QBSDEF qbs_bytes_io_t qbs_bytes_writer(uint8_t *buffer, uint64_t size);

#endif // !QBS_H_

#ifndef QBS_IMPL

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#define qbs_io_min(a, b) (((a) < (b)) ? (a) : (b))

QBSDEF qbs_io_respose_t qbs_io_invalid_rw(void *ctx, uint8_t *bytes, uint64_t size) {
  assert(0 && "unreachable");
  return (qbs_io_respose_t){.err = qbs_io_err_invalid_method};
}

QBSDEF uint16_t qbs_io_invalid_close(void *ctx) {
  assert(0 && "unreachable");
  return qbs_io_err_invalid_method;
}

QBSDEF const uint8_t *qbs_io_error_to_string(int16_t code) {
  switch (code) {
  case qbs_io_err_eof:
    return (uint8_t *)"EOF";
  case qbs_io_err_unexpected_eof:
    return (uint8_t *)"unexpected EOF";
  case qbs_io_err_no_progress:
    return (uint8_t *)"no progress";
  case qbs_io_err_short_buffer:
    return (uint8_t *)"short buffer";
  }
  assert(0 && "unreachable");
  return (uint8_t *)"unreachable";
}

QBSDEF qbs_io_respose_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz) {
  assert(src != 0);
  assert(dst != 0);
  assert(sz != 0);

  qbs_io_respose_t rn, wn;
  uint64_t ttl = 0;

  while (true) {
    rn = src->read(src->ctx, buf, sz);
    if (rn.err == qbs_io_err_eof)
      break;
    if (rn.err != qbs_io_err_null)
      return rn;

    wn = dst->write(dst->ctx, buf, rn.n);
    if (wn.err != qbs_io_err_null)
      return wn;

    if (ttl == UINT64_MAX)
      return (qbs_io_respose_t){
          .err = qbs_io_err_long_buffer,
          .n = ttl,
      };

    ttl += wn.n;
  }
  return (qbs_io_respose_t){
      .err = qbs_io_err_null,
      .n = ttl,
  };
}

QBSDEF qbs_io_respose_t qbs_io_copy(qbs_io_t *src, qbs_io_t *dst) {
  uint8_t mid[512] = {0};
  return qbs_io_copy_buffer(src, dst, mid, sizeof(mid));
}

QBSDEF qbs_io_respose_t qbs_io_limit_read(qbs_limit_ctx *ltx, uint8_t *buf, uint64_t sz) {
  assert(buf != 0);
  assert(sz != 0);

  assert(ltx->done <= ltx->limit);

  if (ltx->is_completed)
    return (qbs_io_respose_t){
        .err = qbs_io_err_no_progress,
        .n = 0,
    };

  if (ltx->done == ltx->limit) {
    ltx->is_completed = true;
    return (qbs_io_respose_t){
        .err = qbs_io_err_eof,
        .n = ltx->done,
    };
  }

  uint64_t rem = ltx->limit - ltx->done;
  sz = qbs_io_min(rem, sz);
  qbs_io_respose_t rn = ltx->r->read(ltx->r->ctx, buf, sz);
  if (rn.err == qbs_io_err_eof)
    return (qbs_io_respose_t){
        .err = qbs_io_err_unexpected_eof,
        .n = rn.n,
    };
  if (rn.err == qbs_io_err_null) {
    if (ltx->done == UINT64_MAX)
      return (qbs_io_respose_t){
          .err = qbs_io_err_long_buffer,
      };
    ltx->done += rn.n;
  }
  return rn;
}

QBSDEF qbs_io_limit_t qbs_io_add_limit(qbs_io_t *r, uint64_t limit) {
  assert(r != 0);
  assert(limit != 0);

  return (qbs_io_limit_t){
      .io =
          {
              .read = (qbs_io_read)qbs_io_limit_read,
              .write = qbs_io_invalid_rw,
              .close = qbs_io_invalid_close,
          },
      .ctx =
          {

              .limit = limit,
              .done = 0,
              .r = r,
              .is_completed = false,
          },
  };
}

QBSDEF qbs_io_respose_t qbs_io_copy_n(qbs_io_t *src, qbs_io_t *dst, uint64_t n) {
  qbs_io_limit_t l = qbs_io_add_limit(src, n);
  return qbs_io_copy((qbs_io_t *)&l, dst);
}

QBSDEF qbs_io_respose_t qbs_io_read_at_least(qbs_io_t *r, uint8_t *b, uint64_t sz, uint64_t min) {
  assert(r != 0);
  assert(b != 0);
  assert(sz != 0);

  if (min > sz)
    return (qbs_io_respose_t){
        .err = qbs_io_err_short_buffer,
        .n = 0,
    };

  qbs_io_respose_t rn = r->read(r->ctx, b, sz);
  if (rn.err != qbs_io_err_null)
    return rn;
  if (rn.n < min)
    return (qbs_io_respose_t){
        .err = qbs_io_err_unexpected_eof,
        .n = rn.n,
    };

  return rn;
}

QBSDEF qbs_io_respose_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz) {
  qbs_io_respose_t result = qbs_io_read_at_least(r, b, sz, sz);
  return result;
}

QBSDEF qbs_io_respose_t qbs_file_close(qbs_file_ctx_t *ctx) {
  return (qbs_io_respose_t){
      .err = close(ctx->fd),
  };
}

QBSDEF qbs_io_respose_t qbs_file_read(qbs_file_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_RDONLY || ctx->mode == O_RDWR);

  uint64_t res = read(ctx->fd, b, sz);
  if (res == 0)
    return (qbs_io_respose_t){
        .err = qbs_io_err_eof,
        .n = 0,
    };
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_io_respose_t qbs_file_write(qbs_file_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_WRONLY || ctx->mode == O_RDWR);

  uint64_t res = write(ctx->fd, b, sz);
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_io_file_t qbs_file_open(char *filename, int mode) {
  int fd = open(filename, mode, 0644);
  if (fd < 0)
    return (qbs_io_file_t){.err = fd};

  qbs_io_read rcb = (mode == O_RDWR || mode == O_RDONLY) ? (qbs_io_read)qbs_file_read : qbs_io_invalid_rw;
  qbs_io_write wcb = (mode == O_RDWR || mode == O_WRONLY) ? (qbs_io_write)qbs_file_write : qbs_io_invalid_rw;

  return (qbs_io_file_t){
      .io =
          {
              .read = rcb,
              .write = wcb,
              .close = (qbs_io_close)qbs_file_close,
          },
      .ctx =
          {
              .filename = filename,
              .mode = mode,
              .fd = fd,
          },
      .err = 0,
  };
}

QBSDEF qbs_io_respose_t qbs_tcp_close(qbs_tcp_ctx_t *ctx) {
  return (qbs_io_respose_t){
      .err = close(ctx->sock),
  };
}

QBSDEF qbs_io_respose_t qbs_tcp_read(qbs_tcp_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t res = read(ctx->sock, b, sz);
  if (res == 0)
    return (qbs_io_respose_t){
        .err = qbs_io_err_eof,
        .n = 0,
    };
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_io_respose_t qbs_tcp_write(qbs_tcp_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t res = write(ctx->sock, b, sz);
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_io_tcp_t qbs_tcp_dail(char *address, uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return (qbs_io_tcp_t){.err = sock};

  struct sockaddr_in seradr;
  seradr.sin_family = AF_INET;
  seradr.sin_port = htons(port);
  inet_pton(AF_INET, address, &seradr.sin_addr);

  int res = connect(sock, (struct sockaddr *)&seradr, sizeof(seradr));
  if (res < 0)
    return (qbs_io_tcp_t){.err = res};

  qbs_io_t io = (qbs_io_t){};
  return (qbs_io_tcp_t){
      .io =
          {
              .read = (qbs_io_read)qbs_tcp_read,
              .write = (qbs_io_write)qbs_tcp_write,
              .close = (qbs_io_close)qbs_tcp_close,
          },
      .ctx =
          {
              .address = address,
              .port = port,
              .sock = sock,

          },
      .err = 0,
  };
}

QBSDEF qbs_tcp_listener_t qbs_tcp_listen(char *address, uint16_t port) {
  int sock, res;
  struct sockaddr_in addr;
  int opt = 1;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == 0) {
    return (qbs_tcp_listener_t){.err = sock};
  }
  res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  if (res != 0) {
    close(sock);
    return (qbs_tcp_listener_t){.err = 1};
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  res = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (res < 0) {
    close(sock);
    return (qbs_tcp_listener_t){.err = res};
  }
  res = listen(sock, 4);
  if (res < 0) {
    close(sock);
    return (qbs_tcp_listener_t){.err = res};
  }
  return (qbs_tcp_listener_t){sock, addr, 0};
}

QBSDEF qbs_io_tcp_t qbs_tcp_accept(qbs_tcp_listener_t *l) {
  int addrlen = sizeof(l->address);
  int sock = accept(l->sock, (struct sockaddr *)&l->address, (socklen_t *)&addrlen);
  if (sock <= 0) {
    return (qbs_io_tcp_t){.err = sock};
  }
  return (qbs_io_tcp_t){
      .io =
          {
              .read = (qbs_io_read)qbs_tcp_read,
              .write = (qbs_io_write)qbs_tcp_write,
              .close = (qbs_io_close)qbs_tcp_close,
          },
      .ctx = {.sock = sock},
      .err = 0,
  };
}

QBSDEF qbs_io_respose_t qbs_bytes_read(qbs_bytes_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  if (ctx->is_completed)
    return (qbs_io_respose_t){
        .err = qbs_io_err_no_progress,
        .n = 0,
    };

  if (ctx->capacity == ctx->offset) {
    ctx->is_completed = true;
    return (qbs_io_respose_t){
        .err = qbs_io_err_eof,
        .n = 0,
    };
  }

  sz = qbs_io_min(sz, ctx->capacity - ctx->offset);
  for (size_t i = 0; i < sz; i++)
    b[i] = ctx->buffer[ctx->offset++];

  return (qbs_io_respose_t){
      .err = qbs_io_err_null,
      .n = sz,
  };
}

QBSDEF qbs_io_respose_t qbs_bytes_write(qbs_bytes_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(sz <= ctx->capacity - ctx->offset);

  for (size_t i = 0; i < sz; i++)
    ctx->buffer[ctx->offset++] = b[i];

  return (qbs_io_respose_t){
      .err = qbs_io_err_null,
      .n = sz,
  };
}

QBSDEF qbs_bytes_io_t qbs_bytes_reader(uint8_t *buffer, uint64_t size) {
  assert(buffer != 0);
  assert(size != 0);

  return (qbs_bytes_io_t){
      .io =
          {
              .read = (qbs_io_read)qbs_bytes_read,
              .write = qbs_io_invalid_rw,
              .close = qbs_io_invalid_close,
          },
      .ctx =
          {
              .offset = 0,
              .capacity = size,
              .buffer = buffer,
              .is_completed = false,
          },
  };
}

QBSDEF qbs_bytes_io_t qbs_bytes_writer(uint8_t *buffer, uint64_t size) {
  assert(buffer != 0);
  assert(size != 0);

  return (qbs_bytes_io_t){
      .io =
          {
              .read = qbs_io_invalid_rw,
              .write = (qbs_io_write)qbs_bytes_write,
              .close = qbs_io_invalid_close,
          },
      .ctx =
          {
              .offset = 0,
              .capacity = size,
              .buffer = buffer,
              .is_completed = false,
          },
  };
}

#endif
