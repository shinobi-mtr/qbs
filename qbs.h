#ifndef QBS_H_
#define QBS_H_

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

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
} qbs_result_t;

typedef qbs_result_t (*qbs_io_read)(void *ctx, uint8_t *bytes, uint64_t size);
typedef qbs_result_t (*qbs_io_write)(void *ctx, uint8_t *bytes, uint64_t size);
typedef uint16_t (*qbs_io_close)(void *ctx);

typedef struct {
  qbs_io_read read;
  qbs_io_write write;
  qbs_io_close close;
} qbs_io_t;

typedef struct {
} qbs_limit_ctx;

typedef struct {
  qbs_io_t io;
  uint64_t limit;
  uint64_t done;
  qbs_io_t *r;
  bool is_completed;
} qbs_limit_t;

typedef struct {
  qbs_io_t io;
  const char *filename;
  const int mode;
  int fd;
  int err;
} qbs_file_t;

typedef struct {
  qbs_io_t io;
  const char *address;
  const uint16_t port;
  int sock;
  int err;
} qbs_sock_t;

typedef struct {
  int sock;
  struct sockaddr_in address;
  int err;
} qbs_listener_t;

typedef struct {
  qbs_io_t io;
  uint64_t offset;
  const uint64_t capacity;
  uint8_t *buffer;
  bool is_completed;
} qbs_bytes_t;

QBSDEF const uint8_t *qbs_io_error_to_string(int16_t code);

// IO Functions
QBSDEF qbs_result_t qbs_io_copy(qbs_io_t *src, qbs_io_t *dst);
QBSDEF qbs_result_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz);
QBSDEF qbs_result_t qbs_io_copy_n(qbs_io_t *src, qbs_io_t *dst, uint64_t n);
QBSDEF qbs_result_t qbs_io_read_at_least(qbs_io_t *r, uint8_t *b, uint64_t sz, uint64_t min);
QBSDEF qbs_result_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz);

// Reader/Writer creation
QBSDEF qbs_limit_t qbs_io_add_limit(qbs_io_t *r, uint64_t limit);
QBSDEF qbs_bytes_t qbs_bytes_reader(uint8_t *buffer, uint64_t size);
QBSDEF qbs_bytes_t qbs_bytes_writer(uint8_t *buffer, uint64_t size);
QBSDEF qbs_file_t qbs_file_open(const char *filename, int mode);
QBSDEF qbs_sock_t qbs_tcp_accept(qbs_listener_t *l);
QBSDEF qbs_sock_t qbs_tcp_dail(const char *address, uint16_t port);
QBSDEF qbs_sock_t qbs_http_get(const char *address, uint16_t port, const char *route, uint16_t rsz, const char *header, uint32_t hsz);
QBSDEF qbs_sock_t qbs_http_post(const char *address, uint16_t port, const char *route, uint16_t rsz, const char *header, uint32_t hsz, qbs_io_t *reader);

QBSDEF qbs_listener_t qbs_tcp_listen(const char *address, uint16_t port);

#endif // !QBS_H_

#ifdef QBS_IMPL

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#define qbs_io_min(a, b) (((a) < (b)) ? (a) : (b))

QBSDEF qbs_result_t qbs_io_invalid_rw(void *ctx, uint8_t *bytes, uint64_t size) {
  assert(0 && "unreachable");
  return (qbs_result_t){.err = qbs_io_err_invalid_method};
}

QBSDEF uint16_t qbs_io_invalid_close(void *ctx) {
  assert(0 && "unreachable");
  return qbs_io_err_invalid_method;
}

QBSDEF const uint8_t *qbs_io_error_to_string(int16_t code) {
  switch (code) {
  case qbs_io_err_null:
    return (uint8_t *)"No error";
  case qbs_io_err_eof:
    return (uint8_t *)"EOF";
  case qbs_io_err_unexpected_eof:
    return (uint8_t *)"unexpected EOF";
  case qbs_io_err_no_progress:
    return (uint8_t *)"no progress";
  case qbs_io_err_short_buffer:
    return (uint8_t *)"short buffer";
  case qbs_io_err_long_buffer:
    return (uint8_t *)"log buffer";
  case qbs_io_err_invalid_method:
    return (uint8_t *)"invalid method";
  }
  assert(0 && "unreachable");
  return (uint8_t *)"unreachable";
}

QBSDEF qbs_result_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz) {
  assert(src != 0);
  assert(dst != 0);
  assert(sz != 0);

  qbs_result_t rn, wn;
  uint64_t ttl = 0;

  while (true) {
    rn = src->read(src, buf, sz);
    if (rn.err == qbs_io_err_eof)
      break;
    if (rn.err != qbs_io_err_null)
      return rn;

    wn = dst->write(dst, buf, rn.n);
    if (wn.err != qbs_io_err_null)
      return wn;

    if (ttl == UINT64_MAX)
      return (qbs_result_t){
          .err = qbs_io_err_long_buffer,
          .n = ttl,
      };

    ttl += wn.n;
  }
  return (qbs_result_t){
      .err = qbs_io_err_null,
      .n = ttl,
  };
}

QBSDEF qbs_result_t qbs_io_copy(qbs_io_t *src, qbs_io_t *dst) {
  uint8_t mid[512] = {0};
  return qbs_io_copy_buffer(src, dst, mid, sizeof(mid));
}

QBSDEF qbs_result_t qbs_io_limit_read(qbs_limit_t *ltx, uint8_t *buf, uint64_t sz) {
  assert(buf != 0);
  assert(sz != 0);

  assert(ltx->done <= ltx->limit);

  if (ltx->is_completed)
    return (qbs_result_t){
        .err = qbs_io_err_no_progress,
        .n = 0,
    };

  if (ltx->done == ltx->limit) {
    ltx->is_completed = true;
    return (qbs_result_t){
        .err = qbs_io_err_eof,
        .n = ltx->done,
    };
  }

  uint64_t rem = ltx->limit - ltx->done;
  sz = qbs_io_min(rem, sz);
  qbs_result_t rn = ltx->r->read(ltx->r, buf, sz);
  if (rn.err == qbs_io_err_eof)
    return (qbs_result_t){
        .err = qbs_io_err_unexpected_eof,
        .n = rn.n,
    };
  if (rn.err == qbs_io_err_null) {
    if (ltx->done == UINT64_MAX)
      return (qbs_result_t){
          .err = qbs_io_err_long_buffer,
      };
    ltx->done += rn.n;
  }
  return rn;
}

QBSDEF qbs_limit_t qbs_io_add_limit(qbs_io_t *r, uint64_t limit) {
  assert(r != 0);
  assert(limit != 0);

  return (qbs_limit_t){
      .io =
          {
              .read = (qbs_io_read)qbs_io_limit_read,
              .write = qbs_io_invalid_rw,
              .close = qbs_io_invalid_close,
          },

      .limit = limit,
      .done = 0,
      .r = r,
      .is_completed = false,
  };
}

QBSDEF qbs_result_t qbs_io_copy_n(qbs_io_t *src, qbs_io_t *dst, uint64_t n) {
  qbs_limit_t l = qbs_io_add_limit(src, n);
  return qbs_io_copy((qbs_io_t *)&l, dst);
}

QBSDEF qbs_result_t qbs_io_read_at_least(qbs_io_t *r, uint8_t *b, uint64_t sz, uint64_t min) {
  assert(r != 0);
  assert(b != 0);
  assert(sz != 0);

  if (min > sz)
    return (qbs_result_t){
        .err = qbs_io_err_short_buffer,
        .n = 0,
    };

  qbs_result_t rn = r->read(r, b, sz);
  if (rn.err != qbs_io_err_null)
    return rn;
  if (rn.n < min)
    return (qbs_result_t){
        .err = qbs_io_err_unexpected_eof,
        .n = rn.n,
    };

  return rn;
}

QBSDEF qbs_result_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz) {
  qbs_result_t result = qbs_io_read_at_least(r, b, sz, sz);
  return result;
}

QBSDEF qbs_result_t qbs_file_close(qbs_file_t *ctx) {
  return (qbs_result_t){
      .err = close(ctx->fd),
  };
}

QBSDEF qbs_result_t qbs_file_read(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_RDONLY || ctx->mode == O_RDWR);

  uint64_t res = read(ctx->fd, b, sz);
  if (res == 0)
    return (qbs_result_t){
        .err = qbs_io_err_eof,
        .n = 0,
    };
  return (qbs_result_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_result_t qbs_file_write(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_WRONLY || ctx->mode == O_RDWR);

  uint64_t res = write(ctx->fd, b, sz);
  return (qbs_result_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_file_t qbs_file_open(const char *filename, int mode) {
  int fd = open(filename, mode, 0644);
  if (fd < 0)
    return (qbs_file_t){.err = fd};

  qbs_io_read rcb = (mode == O_RDWR || mode == O_RDONLY) ? (qbs_io_read)qbs_file_read : qbs_io_invalid_rw;
  qbs_io_write wcb = (mode == O_RDWR || mode == O_WRONLY) ? (qbs_io_write)qbs_file_write : qbs_io_invalid_rw;

  return (qbs_file_t){
      .io =
          {
              .read = rcb,
              .write = wcb,
              .close = (qbs_io_close)qbs_file_close,
          },
      .filename = filename,
      .mode = mode,
      .fd = fd,
      .err = 0,
  };
}

QBSDEF qbs_result_t qbs_tcp_close(qbs_sock_t *ctx) {
  return (qbs_result_t){
      .err = close(ctx->sock),
  };
}

QBSDEF qbs_result_t qbs_tcp_read(qbs_sock_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t res = read(ctx->sock, b, sz);
  if (res == 0)
    return (qbs_result_t){
        .err = qbs_io_err_eof,
        .n = 0,
    };
  return (qbs_result_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_result_t qbs_tcp_write(qbs_sock_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t res = write(ctx->sock, b, sz);
  return (qbs_result_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

QBSDEF qbs_sock_t qbs_tcp_dail(const char *address, uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    return (qbs_sock_t){.err = sock};

  struct sockaddr_in seradr;
  seradr.sin_family = AF_INET;
  seradr.sin_port = htons(port);
  inet_pton(AF_INET, address, &seradr.sin_addr);

  int res = connect(sock, (struct sockaddr *)&seradr, sizeof(seradr));
  if (res < 0)
    return (qbs_sock_t){.err = res};

  qbs_io_t io = (qbs_io_t){};
  return (qbs_sock_t){
      .io =
          {
              .read = (qbs_io_read)qbs_tcp_read,
              .write = (qbs_io_write)qbs_tcp_write,
              .close = (qbs_io_close)qbs_tcp_close,
          },
      .address = address,
      .port = port,
      .sock = sock,
      .err = 0,
  };
}

QBSDEF qbs_listener_t qbs_tcp_listen(const char *address, uint16_t port) {
  int sock, res;
  struct sockaddr_in addr;
  int opt = 1;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == 0) {
    return (qbs_listener_t){.err = sock};
  }
  res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  if (res != 0) {
    close(sock);
    return (qbs_listener_t){.err = 1};
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  res = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (res < 0) {
    close(sock);
    return (qbs_listener_t){.err = res};
  }
  res = listen(sock, 4);
  if (res < 0) {
    close(sock);
    return (qbs_listener_t){.err = res};
  }
  return (qbs_listener_t){sock, addr, 0};
}

QBSDEF qbs_sock_t qbs_tcp_accept(qbs_listener_t *l) {
  int addrlen = sizeof(l->address);
  int sock = accept(l->sock, (struct sockaddr *)&l->address, (socklen_t *)&addrlen);
  if (sock <= 0) {
    return (qbs_sock_t){.err = sock};
  }
  return (qbs_sock_t){
      .io =
          {
              .read = (qbs_io_read)qbs_tcp_read,
              .write = (qbs_io_write)qbs_tcp_write,
              .close = (qbs_io_close)qbs_tcp_close,
          },
      .sock = sock,
      .err = 0,
  };
}

QBSDEF qbs_result_t qbs_bytes_read(qbs_bytes_t *ctx, uint8_t *b, uint64_t sz) {
  if (ctx->is_completed)
    return (qbs_result_t){
        .err = qbs_io_err_no_progress,
        .n = 0,
    };

  if (ctx->capacity == ctx->offset) {
    ctx->is_completed = true;
    return (qbs_result_t){
        .err = qbs_io_err_eof,
        .n = 0,
    };
  }

  sz = qbs_io_min(sz, ctx->capacity - ctx->offset);
  for (size_t i = 0; i < sz; i++)
    b[i] = ctx->buffer[ctx->offset++];

  return (qbs_result_t){
      .err = qbs_io_err_null,
      .n = sz,
  };
}

QBSDEF qbs_result_t qbs_bytes_write(qbs_bytes_t *ctx, uint8_t *b, uint64_t sz) {
  assert(sz <= ctx->capacity - ctx->offset);

  for (size_t i = 0; i < sz; i++)
    ctx->buffer[ctx->offset++] = b[i];

  return (qbs_result_t){
      .err = qbs_io_err_null,
      .n = sz,
  };
}

QBSDEF qbs_bytes_t qbs_bytes_reader(uint8_t *buffer, uint64_t size) {
  assert(buffer != 0);
  assert(size != 0);

  return (qbs_bytes_t){
      .io =
          {
              .read = (qbs_io_read)qbs_bytes_read,
              .write = qbs_io_invalid_rw,
              .close = qbs_io_invalid_close,
          },
      .offset = 0,
      .capacity = size,
      .buffer = buffer,
      .is_completed = false,
  };
}

QBSDEF qbs_bytes_t qbs_bytes_writer(uint8_t *buffer, uint64_t size) {
  assert(buffer != 0);
  assert(size != 0);

  return (qbs_bytes_t){
      .io =
          {
              .read = qbs_io_invalid_rw,
              .write = (qbs_io_write)qbs_bytes_write,
              .close = qbs_io_invalid_close,
          },
      .offset = 0,
      .capacity = size,
      .buffer = buffer,
      .is_completed = false,
  };
}

QBSDEF qbs_sock_t qbs_http_get(const char *address, uint16_t port, const char *route, uint16_t rsz, const char *header, uint32_t hsz) {
  qbs_result_t r;

  qbs_sock_t s = qbs_tcp_dail(address, port);
  if (s.err != 0)
    return s;

  r = s.io.write(&s, (uint8_t *)"GET ", 4);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)route, rsz);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)" HTTP/1.1\r\n", 11);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)header, hsz);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)"\r\n", 2);
  if (r.err != 0)
    goto err;

  s.io.write = qbs_io_invalid_rw;
  return s;

err:
  s.io.close(&s);
  s.err = r.err;
  return s;
}

QBSDEF qbs_sock_t qbs_http_post(const char *address, uint16_t port, const char *route, uint16_t rsz, const char *header, uint32_t hsz, qbs_io_t *reader) {
  qbs_result_t r;

  qbs_sock_t s = qbs_tcp_dail(address, port);
  if (s.err != 0)
    return s;

  r = s.io.write(&s, (uint8_t *)"POST ", 5);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)route, rsz);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)" HTTP/1.1\r\n", 11);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)header, hsz);
  if (r.err != 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)"\r\n", 2);
  if (r.err != 0)
    goto err;

  r = qbs_io_copy(reader, &s.io);
  if (r.err != 0)
    goto err;

  s.io.write = qbs_io_invalid_rw;
  return s;

err:
  s.io.close(&s);
  s.err = r.err;
  return s;
}

#endif
