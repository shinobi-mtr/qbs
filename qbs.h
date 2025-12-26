// Copyright (c) 2025 sultan al-motiri <education.sultan.mo@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef QBS_H_
#define QBS_H_

#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#ifndef QBSDEF
#define QBSDEF static inline
#endif

#define QBS_EOF -1
#define QBS_UNXEOF 1
#define QBS_NOPROG 2
#define QBS_TOSMALL 3
#define QBS_TOBIG 4
#define QBS_NOMETH 5

typedef struct {
  bool err;
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
  bool err;
} qbs_file_t;

typedef struct {
  qbs_io_t io;
  const char *address;
  const uint16_t port;
  int sock;
  bool err;
} qbs_sock_t;

typedef struct {
  int sock;
  struct sockaddr_in address;
  bool err;
} qbs_listener_t;

typedef struct {
  qbs_io_t io;
  uint64_t offset;
  const uint64_t capacity;
  uint8_t *buffer;
  bool is_completed;
} qbs_bytes_t;

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
  errno = QBS_NOMETH;
  return (qbs_result_t){.err = true};
}

QBSDEF uint16_t qbs_io_invalid_close(void *ctx) {
  assert(0 && "unreachable");
  return QBS_NOMETH;
}

QBSDEF qbs_result_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz) {
  assert(src != 0);
  assert(dst != 0);
  assert(sz != 0);

  qbs_result_t rn, wn;
  uint64_t ttl = 0;

  while (true) {
    rn = src->read(src, buf, sz);
    if (rn.err == true && errno == QBS_EOF)
      break;
    if (rn.err == true)
      return (qbs_result_t){
          .err = true,
          .n = ttl,
      };

    wn = dst->write(dst, buf, rn.n);
    if (wn.err == true)
      return (qbs_result_t){
          .err = true,
          .n = ttl,
      };

    if (UINT64_MAX - ttl < rn.n) {
      errno = QBS_TOBIG;
      return (qbs_result_t){
          .err = true,
          .n = ttl,
      };
    }
    ttl += wn.n;
    continue;
  }
  return (qbs_result_t){
      .err = false,
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

  if (ltx->is_completed) {
    errno = QBS_NOPROG;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  if (ltx->done == ltx->limit) {
    ltx->is_completed = true;
    errno = QBS_EOF;
    return (qbs_result_t){
        .err = true,
        .n = ltx->done,
    };
  }

  uint64_t rem = ltx->limit - ltx->done;
  sz = qbs_io_min(rem, sz);
  qbs_result_t rn = ltx->r->read(ltx->r, buf, sz);
  if (rn.err == true && errno == QBS_EOF) {
    ltx->is_completed = true;
    return (qbs_result_t){
        .err = true,
        .n = rn.n,
    };
  }

  if (rn.err == true)
    return rn;

  if (UINT64_MAX - ltx->done < rn.n) {
    errno = QBS_TOBIG;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  ltx->done += rn.n;
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
  qbs_result_t rn = {
      .err = false,
      .n = 0,
  };

  if (min > sz) {
    errno = QBS_TOSMALL;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  uint64_t ttl = 0;
  while (ttl < min) {
    rn = r->read(r, b, sz);
    if (rn.err == true && errno == QBS_EOF)
      break;
    if (rn.err == true)
      return (qbs_result_t){
          .err = true,
          .n = ttl,
      };
    if (UINT64_MAX - ttl < rn.n) {
      errno = QBS_TOBIG;
      return (qbs_result_t){
          .err = true,
          .n = ttl,
      };
    }
    ttl += rn.n;
  }
  if (ttl < min) {
    errno = QBS_UNXEOF;
    return (qbs_result_t){
        .err = true,
        .n = ttl,
    };
  }
  return (qbs_result_t){
      .err = false,
      .n = ttl,
  };
}

QBSDEF qbs_result_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz) {
  qbs_result_t result = qbs_io_read_at_least(r, b, sz, sz);
  return result;
}

QBSDEF uint16_t qbs_file_close(qbs_file_t *ctx) { return close(ctx->fd); }

QBSDEF qbs_result_t qbs_file_read(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_RDONLY || ctx->mode == O_RDWR);
  assert(sz > 0);

  int64_t res = read(ctx->fd, b, sz);
  if (res == 0) {
    errno = QBS_EOF;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  if (res < 0)
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };

  return (qbs_result_t){
      .err = false,
      .n = (uint64_t)res,
  };
}

QBSDEF qbs_result_t qbs_file_write(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_WRONLY || ctx->mode == O_RDWR);
  assert(sz > 0);

  int64_t res = write(ctx->fd, b, sz);
  if (res == -1) {
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  return (qbs_result_t){
      .err = false,
      .n = (uint64_t)res,
  };
}

QBSDEF qbs_file_t qbs_file_open(const char *filename, int mode) {
  int fd = open(filename, mode, 0644);
  if (fd == -1)
    return (qbs_file_t){.err = true};

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
      .err = false,
  };
}

QBSDEF uint16_t qbs_tcp_close(qbs_sock_t *ctx) { return close(ctx->sock); }

QBSDEF qbs_result_t qbs_tcp_read(qbs_sock_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t res = read(ctx->sock, b, sz);
  if (res == 0) {
    errno = QBS_EOF;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }
  if (res == -1)
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  return (qbs_result_t){
      .err = false,
      .n = res,
  };
}

QBSDEF qbs_result_t qbs_tcp_write(qbs_sock_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t ttl = sz;
  while (sz != 0) {
    int64_t res = write(ctx->sock, b, sz);
    if (res == -1)
      return (qbs_result_t){
          .err = true,
          .n = 0,
      };
    sz -= res;
    b += res;
  }
  return (qbs_result_t){
      .err = false,
      .n = ttl,
  };
}

QBSDEF qbs_sock_t qbs_tcp_dail(const char *address, uint16_t port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    return (qbs_sock_t){.err = true};

  struct sockaddr_in seradr;
  seradr.sin_family = AF_INET;
  seradr.sin_port = htons(port);
  inet_pton(AF_INET, address, &seradr.sin_addr);

  int res = connect(sock, (struct sockaddr *)&seradr, sizeof(seradr));
  if (res != 0)
    return (qbs_sock_t){.err = true};

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
      .err = false,
  };
}

QBSDEF qbs_listener_t qbs_tcp_listen(const char *address, uint16_t port) {
  int sock, res;
  struct sockaddr_in addr;
  int opt = 1;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1) {
    return (qbs_listener_t){.err = true};
  }
  res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
  if (res != 0) {
    close(sock);
    return (qbs_listener_t){.err = true};
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  res = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
  if (res != 0) {
    close(sock);
    return (qbs_listener_t){.err = true};
  }
  res = listen(sock, 4);
  if (res != 0) {
    close(sock);
    return (qbs_listener_t){.err = true};
  }
  return (qbs_listener_t){
      .sock = sock,
      .address = addr,
      .err = false,
  };
}

QBSDEF qbs_sock_t qbs_tcp_accept(qbs_listener_t *l) {
  int addrlen = sizeof(l->address);
  int sock = accept(l->sock, (struct sockaddr *)&l->address, (socklen_t *)&addrlen);
  if (sock == -1)
    return (qbs_sock_t){.err = errno};

  return (qbs_sock_t){
      .io =
          {
              .read = (qbs_io_read)qbs_tcp_read,
              .write = (qbs_io_write)qbs_tcp_write,
              .close = (qbs_io_close)qbs_tcp_close,
          },
      .sock = sock,
      .err = false,
  };
}

QBSDEF qbs_result_t qbs_bytes_read(qbs_bytes_t *ctx, uint8_t *b, uint64_t sz) {
  if (ctx->is_completed) {
    errno = QBS_NOPROG;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  if (ctx->capacity == ctx->offset) {
    ctx->is_completed = true;
    errno = QBS_EOF;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  sz = qbs_io_min(sz, ctx->capacity - ctx->offset);
  for (size_t i = 0; i < sz; i++)
    b[i] = ctx->buffer[ctx->offset++];

  return (qbs_result_t){
      .err = false,
      .n = sz,
  };
}

QBSDEF qbs_result_t qbs_bytes_write(qbs_bytes_t *ctx, uint8_t *b, uint64_t sz) {
  if (ctx->capacity - ctx->offset < sz) {
    errno = QBS_TOSMALL;
    return (qbs_result_t){
        .err = true,
        .n = 0,
    };
  }

  for (size_t i = 0; i < sz; i++)
    ctx->buffer[ctx->offset++] = b[i];

  return (qbs_result_t){
      .err = false,
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
  if (s.err == true)
    return s;

  r = s.io.write(&s, (uint8_t *)"GET ", 4);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)route, rsz);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)" HTTP/1.1\r\n", 11);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)header, hsz);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)"\r\n", 2);
  if (r.err == true)
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
  if (s.err == true)
    return s;

  r = s.io.write(&s, (uint8_t *)"POST ", 5);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)route, rsz);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)" HTTP/1.1\r\n", 11);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)header, hsz);
  if (r.err == true)
    goto err;

  r = s.io.write(&s, (uint8_t *)"\r\n", 2);
  if (r.err == true)
    goto err;

  r = qbs_io_copy(reader, &s.io);
  if (r.err == true)
    goto err;

  s.io.write = qbs_io_invalid_rw;
  return s;

err:
  s.io.close(&s);
  s.err = r.err;
  return s;
}

#endif
