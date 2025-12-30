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

/*
 * @brief Error codes that errno will be set to if an error is detected by the library.
 */
typedef enum {
  QBS_EOF = -1,
  QBS_UNXEOF = 1,
  QBS_NOPROG = 2,
  QBS_TOSMALL = 3,
  QBS_TOBIG = 4,
  QBS_NOMETH = 5,
  QBS_PARTW = 6,
} qbs_error_t;

typedef uint64_t (*qbs_io_read)(void *ctx, uint8_t *bytes, uint64_t size);
typedef uint64_t (*qbs_io_write)(void *ctx, uint8_t *bytes, uint64_t size);
typedef uint16_t (*qbs_io_close)(void *ctx);

/*
 * @brief QBS object. This struct must exist within structs intended for use as stream sources.
 */
typedef struct {
  qbs_io_read read;   // If the stream source does not implement a reader, set this to qbs_io_invalid_rw.
  qbs_io_write write; // If the stream source does not implement a writer, set this to qbs_io_invalid_rw.
  qbs_io_close close; // If the stream source does not implement a closer, set this to qbs_io_invalid_close.
} qbs_io_t;

/*
 * @brief A stream source that limits another stream source's reader to a given byte count.
 */
typedef struct {
  qbs_io_t io;       // QBS object (reader implemented only).
  uint64_t limit;    // The actual limit the reader is not allowed to bypass.
  uint64_t done;     // Number of bytes processed; errno is set to EOF if this reaches the limit.
  qbs_io_t *r;       // Pointer to the underlying QBS stream source being limited.
  bool is_completed; // True if the limit has been reached.
} qbs_limit_t;

/*
 * @brief A stream source for handling files.
 *
 * @note This struct should only be constructed via qbs_file_open.
 */
typedef struct {
  qbs_io_t io;          // QBS object; reader/writer set to qbs_io_invalid_rw based on open mode.
  const char *filename; // The filename provided by the user.
  const int mode;       // File opening mode provided by the user.
  int fd;               // The file descriptor returned from the open function.
  bool err;             // True if an error occurred; details can be found in errno.
} qbs_file_t;

/*
 * @brief A stream source for handling TCP connections.
 *
 * @note This struct should only be constructed via qbs_tcp_accept or qbs_tcp_dial.
 */
typedef struct {
  qbs_io_t io;         // QBS object.
  const char *address; // The address provided by the user.
  const uint16_t port; // The port provided by the user.
  int sock;            // The file descriptor returned by accept or connect functions.
  bool err;            // True if an error occurred; details can be found in errno.
} qbs_sock_t;

/*
 * @brief TCP listener context.
 *
 * @note This struct should only be constructed via qbs_tcp_listen.
 */
typedef struct {
  int sock;                   // File descriptor returned by the socket function.
  struct sockaddr_in address; // The address to listen on.
  bool err;                   // True if an error occurred; details can be found in errno.
} qbs_listener_t;

/*
 * @brief A stream source for handling byte arrays and buffers.
 *
 * @note This struct should only be constructed via qbs_bytes_reader or qbs_bytes_writer.
 */
typedef struct {
  qbs_io_t io;             // QBS object; read/write methods set based on the construction function.
  uint64_t offset;         // Current offset used to read/write from the correct index.
  const uint64_t capacity; // Buffer size; prevents reading or writing beyond this limit.
  uint8_t *buffer;         // The underlying buffer used for I/O operations.
  bool is_completed;       // True if the offset has reached the capacity.
} qbs_bytes_t;

/*
 * @brief Copies a stream of data from src to dst. Similar to qbs_io_copy_buffer but uses an internal buffer.
 *
 * @param src  QBS IO object implementing the reader interface.
 * @param dst  QBS IO object implementing the writer interface.
 *
 * @return the size of the processed buffer
 * @retval == 0 : if error occurred.
 * @retval != 0 : the lenght of the processed buffer.
 */
QBSDEF uint64_t qbs_io_copy(qbs_io_t *src, qbs_io_t *dst);

/*
 * @brief Copies a stream of data from src to dst using a user-provided buffer.
 *
 * @param src  QBS IO object implementing the reader interface.
 * @param dst  QBS IO object implementing the writer interface.
 * @param buf  Byte array used as the intermediate buffer for copying.
 * @param sz   Size of the provided buffer (buf).
 *
 * @return the size of the processed buffer
 * @retval == 0 : if error occurred.
 * @retval != 0 : the lenght of the processed buffer.
 */
QBSDEF uint64_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz);

/*
 * @brief Copies a stream of data from src to dst with a specific byte limit.
 *
 * @param src  QBS IO object implementing the reader interface.
 * @param dst  QBS IO object implementing the writer interface.
 * @param n    The maximum number of bytes to copy.
 *
 * @return the size of the processed buffer
 * @retval == 0 : if error occurred.
 * @retval == n : the lenght of the processed buffer, which must be n.
 *
 * @note An error is returned if EOF is reached before n bytes are copied.
 */
QBSDEF uint64_t qbs_io_copy_n(qbs_io_t *src, qbs_io_t *dst, uint64_t n);

/*
 * @brief Reads at least 'min' bytes from the data source into the provided buffer.
 *
 * @param r    QBS IO object implementing the reader interface.
 * @param b    Buffer to copy data into.
 * @param sz   Total size of the buffer (b).
 * @param min  Minimum number of bytes to be read.
 *
 * @return the size of the processed buffer
 * @retval == 0 : if error occurred.
 * @retval >= min : the lenght of the processed buffer, which must be 'min'.
 *
 * @note An error is returned if EOF is reached before 'min' bytes are copied.
 */
QBSDEF uint64_t qbs_io_read_at_least(qbs_io_t *r, uint8_t *b, uint64_t sz, uint64_t min);

/*
 * @brief Reads data from the reader until the buffer is full.
 *
 * @param r    QBS IO object implementing the reader interface.
 * @param b    Buffer to copy data into.
 * @param sz   Size of the buffer (b).
 *
 * @return the size of the processed buffer
 * @retval == 0 : if error occurred.
 * @retval == sz : the lenght of the processed buffer, which must be 'sz'.
 *
 * @note Returns an error if EOF is reached before the buffer is filled.
 */
QBSDEF uint64_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz);

/*
 * @brief Creates a new QBS object that limits its reader to a specific byte count.
 *
 * @param r      The source QBS IO object.
 * @param limit  The maximum number of bytes the new reader is allowed to read.
 *
 * @return A qbs_limit_t stream source.
 */
QBSDEF qbs_limit_t qbs_io_add_limit(qbs_io_t *r, uint64_t limit);

/*
 * @brief Creates a new QBS object with a reader for a specific byte buffer.
 *
 * @param buffer Array of bytes to be read.
 * @param size   Size of the provided buffer.
 *
 * @return A qbs_bytes_t stream source.
 */
QBSDEF qbs_bytes_t qbs_bytes_reader(uint8_t *buffer, uint64_t size);

/*
 * @brief Creates a new QBS object with a writer for a specific byte buffer.
 *
 * @param buffer Array of bytes where data will be written.
 * @param size   Maximum capacity of the buffer.
 *
 * @return A qbs_bytes_t stream source.
 */
QBSDEF qbs_bytes_t qbs_bytes_writer(uint8_t *buffer, uint64_t size);

/*
 * @brief Creates a new QBS object for file I/O.
 *
 * @param filename The name of the file to open.
 * @param mode     File opening mode (defines if reader or writer is implemented).
 *
 * @return A qbs_file_t stream source.
 */
QBSDEF qbs_file_t qbs_file_open(const char *filename, int mode);

/*
 * @brief Creates a new QBS object to handle an accepted TCP client connection.
 *
 * @param l  A QBS listener object.
 *
 * @return A qbs_sock_t stream source.
 */
QBSDEF qbs_sock_t qbs_tcp_accept(qbs_listener_t *l);

/*
 * @brief Creates a new QBS object and connects to a remote TCP server.
 *
 * @param address The target server address.
 * @param port    The target server port.
 *
 * @return A qbs_sock_t stream source.
 */
QBSDEF qbs_sock_t qbs_tcp_dial(const char *address, uint16_t port);

/*
 * @brief TBD.
 */
QBSDEF qbs_sock_t qbs_http_get(const char *address, uint16_t port, const char *route, uint16_t rsz, const char *header, uint32_t hsz);

/*
 * @brief TBD.
 */
QBSDEF qbs_sock_t qbs_http_post(const char *address, uint16_t port, const char *route, uint16_t rsz, const char *header, uint32_t hsz, qbs_io_t *reader);

/*
 * @brief Creates a TCP listener that manages client connections as QBS IO objects.
 *
 * @param address The address to bind to.
 * @param port    The port to listen on.
 *
 * @return A QBS listener object.
 */
QBSDEF qbs_listener_t qbs_tcp_listen(const char *address, uint16_t port);

#endif // !QBS_H_

#ifdef QBS_IMPL

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#define qbs_io_min(a, b) (((a) < (b)) ? (a) : (b))

QBSDEF uint64_t qbs_io_invalid_rw(void *ctx, uint8_t *bytes, uint64_t size) {
  assert(0 && "unreachable");
  assert(ctx == 0);
  assert(bytes == 0);
  assert(size == 0);
  errno = QBS_NOMETH;
  return 0;
}

QBSDEF uint16_t qbs_io_invalid_close(void *ctx) {
  assert(0 && "unreachable");
  assert(ctx == 0);
  errno = QBS_NOMETH;
  return 0;
}

QBSDEF uint64_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz) {
  assert(src != 0);
  assert(dst != 0);
  assert(sz != 0);

  uint64_t rn, wn;
  uint64_t ttl = 0;

  while (true) {
    rn = src->read(src, buf, sz);
    if (rn == 0 && errno == QBS_EOF)
      break;
    if (rn == 0)
      return 0;

    wn = dst->write(dst, buf, rn);
    if (wn == 0)
      return 0;

    if (wn != rn) {
      errno = QBS_PARTW;
      return 0;
    }
    if (UINT64_MAX - ttl < wn) {
      errno = QBS_TOBIG;
      return 0;
    }
    ttl += wn;
    continue;
  }
  return ttl;
}

QBSDEF uint64_t qbs_io_copy(qbs_io_t *src, qbs_io_t *dst) {
  uint8_t mid[512] = {0};
  return qbs_io_copy_buffer(src, dst, mid, sizeof(mid));
}

QBSDEF uint64_t qbs_io_limit_read(qbs_limit_t *ltx, uint8_t *buf, uint64_t sz) {
  assert(buf != 0);
  assert(sz != 0);
  assert(ltx->done <= ltx->limit);

  if (ltx->is_completed) {
    errno = QBS_NOPROG;
    return 0;
  }

  if (ltx->done == ltx->limit) {
    ltx->is_completed = true;
    errno = QBS_EOF;
    return 0;
  }

  uint64_t rem = ltx->limit - ltx->done;
  sz = qbs_io_min(rem, sz);
  uint64_t rn = ltx->r->read(ltx->r, buf, sz);
  if (rn == 0 && errno == QBS_EOF) {
    ltx->is_completed = true;
    return 0;
  }
  if (rn == 0)
    return 0;

  if (UINT64_MAX - ltx->done < rn) {
    errno = QBS_TOBIG;
    return 0;
  }

  ltx->done += rn;
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

QBSDEF uint64_t qbs_io_copy_n(qbs_io_t *src, qbs_io_t *dst, uint64_t n) {
  qbs_limit_t l = qbs_io_add_limit(src, n);
  return qbs_io_copy((qbs_io_t *)&l, dst);
}

QBSDEF uint64_t qbs_io_read_at_least(qbs_io_t *r, uint8_t *b, uint64_t sz, uint64_t min) {
  assert(r != 0);
  assert(b != 0);
  assert(sz != 0);

  if (min > sz) {
    errno = QBS_TOSMALL;
    return 0;
  }

  uint64_t ttl = 0;
  while (ttl < min) {
    uint64_t rn = r->read(r, b, sz);
    if (rn == 0 && errno == QBS_EOF)
      break;
    if (rn == 0)
      return 0;

    if (UINT64_MAX - ttl < rn) {
      errno = QBS_TOBIG;
      return 0;
    }
    ttl += rn;
  }
  if (ttl < min) {
    errno = QBS_UNXEOF;
    return 0;
  }
  return ttl;
}

QBSDEF uint64_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz) {
  uint64_t result = qbs_io_read_at_least(r, b, sz, sz);
  return result;
}

QBSDEF uint16_t qbs_file_close(qbs_file_t *ctx) { return close(ctx->fd); }

QBSDEF uint64_t qbs_file_read(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_RDONLY || ctx->mode == O_RDWR);
  assert(sz > 0);

  int64_t res = read(ctx->fd, b, sz);
  if (res == 0) {
    errno = QBS_EOF;
    return 0;
  }

  if (res < 0)
    return 0;

  return res;
}

QBSDEF uint64_t qbs_file_write(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode == O_WRONLY || ctx->mode == O_RDWR);
  assert(sz > 0);

  int64_t res = write(ctx->fd, b, sz);
  if (res == -1)
    return 0;

  return res;
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

QBSDEF uint64_t qbs_tcp_read(qbs_sock_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  int64_t res = read(ctx->sock, b, sz);
  if (res == 0) {
    errno = QBS_EOF;
    return 0;
  }
  if (res == -1)
    return 0;
  return res;
}

QBSDEF uint64_t qbs_tcp_write(qbs_sock_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t ttl = sz;
  while (sz != 0) {
    int64_t res = write(ctx->sock, b, sz);
    if (res == -1)
      return 0;
    sz -= res;
    b += res;
  }
  return ttl;
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
  addr.sin_port = htons(port);
  inet_pton(AF_INET, address, &addr.sin_addr);

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

QBSDEF uint64_t qbs_bytes_read(qbs_bytes_t *ctx, uint8_t *b, uint64_t sz) {
  if (ctx->is_completed) {
    errno = QBS_NOPROG;
    return 0;
  }

  if (ctx->capacity == ctx->offset) {
    ctx->is_completed = true;
    errno = QBS_EOF;
    return 0;
  }

  sz = qbs_io_min(sz, ctx->capacity - ctx->offset);
  for (size_t i = 0; i < sz; i++)
    b[i] = ctx->buffer[ctx->offset++];

  return sz;
}

QBSDEF uint64_t qbs_bytes_write(qbs_bytes_t *ctx, uint8_t *b, uint64_t sz) {
  if (ctx->capacity - ctx->offset < sz) {
    errno = QBS_TOSMALL;
    return 0;
  }

  for (size_t i = 0; i < sz; i++)
    ctx->buffer[ctx->offset++] = b[i];
  return sz;
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
  uint64_t r;

  qbs_sock_t s = qbs_tcp_dail(address, port);
  if (s.err == true)
    return s;

  r = s.io.write(&s, (uint8_t *)"GET ", 4);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)route, rsz);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)" HTTP/1.1\r\n", 11);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)header, hsz);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)"\r\n", 2);
  if (r == 0)
    goto err;

  s.io.write = qbs_io_invalid_rw;
  return s;

err:
  s.io.close(&s);
  s.err = true;
  return s;
}

QBSDEF qbs_sock_t qbs_http_post(const char *address, uint16_t port, const char *route, uint16_t rsz, const char *header, uint32_t hsz, qbs_io_t *reader) {
  uint64_t r;

  qbs_sock_t s = qbs_tcp_dail(address, port);
  if (s.err == true)
    return s;

  r = s.io.write(&s, (uint8_t *)"POST ", 5);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)route, rsz);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)" HTTP/1.1\r\n", 11);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)header, hsz);
  if (r == 0)
    goto err;

  r = s.io.write(&s, (uint8_t *)"\r\n", 2);
  if (r == 0)
    goto err;

  r = qbs_io_copy(reader, &s.io);
  if (r == 0)
    goto err;

  s.io.write = qbs_io_invalid_rw;
  return s;

err:
  s.io.close(&s);
  s.err = true;
  return s;
}

#endif
