#pragma once

#include "io.h"
#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>
#include <unistd.h>

typedef enum { qbs_streams_err_gen } qbs_streams_err;

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

qbs_io_respose_t qbs_file_close(qbs_file_ctx_t *ctx) {
  return (qbs_io_respose_t){
      .err = close(ctx->fd),
  };
}

qbs_io_respose_t qbs_file_read(qbs_file_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode & O_RDONLY || ctx->mode & O_RDWR);

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

qbs_io_respose_t qbs_file_write(qbs_file_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode & O_WRONLY || ctx->mode & O_RDWR);

  uint64_t res = write(ctx->fd, b, sz);
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

qbs_io_file_t qbs_file_open(char *filename, int mode) {
  int fd = open(filename, mode, 0644);
  if (fd < 0)
    return (qbs_io_file_t){.err = fd};

  return (qbs_io_file_t){
      .io =
          {
              .read = (qbs_io_read)qbs_file_read,
              .write = (qbs_io_write)qbs_file_write,
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

qbs_io_respose_t qbs_tcp_close(qbs_tcp_ctx_t *ctx) {
  return (qbs_io_respose_t){
      .err = close(ctx->sock),
  };
}

qbs_io_respose_t qbs_tcp_read(qbs_tcp_ctx_t *ctx, uint8_t *b, uint64_t sz) {
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

qbs_io_respose_t qbs_tcp_write(qbs_tcp_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t res = write(ctx->sock, b, sz);
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}
qbs_io_tcp_t qbs_tcp_dail(char *address, uint16_t port) {
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
