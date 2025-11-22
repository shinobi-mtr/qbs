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

} qbs_file_t;

qbs_io_respose_t qbs_stream_close(qbs_file_t *ctx) {
  return (qbs_io_respose_t){
      .err = close(ctx->fd),
  };
}

qbs_io_respose_t qbs_stream_read(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

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

qbs_io_respose_t qbs_stream_write(qbs_file_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);

  uint64_t res = write(ctx->fd, b, sz);
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

int qbs_file_open(qbs_io_t *io, qbs_file_t *ctx) {
  assert(io != 0);
  assert(ctx != 0);

  ctx->fd = open(ctx->filename, ctx->mode, 0644);
  if (ctx->fd < 0)
    return ctx->fd;

  *io = (qbs_io_t){
      .ctx = ctx,
      .read = (qbs_io_read)qbs_stream_read,
      .write = (qbs_io_write)qbs_stream_write,
      .close = (qbs_io_close)qbs_stream_close,
  };
  return 0;
}

typedef struct {
  const char *ip;
  const uint16_t port;
  int sock;
} qbs_tcp_t;

int qbs_tcp_connect(qbs_io_t *io, qbs_tcp_t *ctx) {
  assert(io != 0);
  assert(ctx != 0);

  ctx->sock = socket(AF_INET, SOCK_STREAM, 0);
  if (ctx->sock < 0)
    return ctx->sock;

  struct sockaddr_in seradr;
  seradr.sin_family = AF_INET;
  seradr.sin_port = htons(ctx->port);
  inet_pton(AF_INET, ctx->ip, &seradr.sin_addr);

  int res = connect(ctx->sock, (struct sockaddr *)&seradr, sizeof(seradr));
  if (res < 0)
    return res;

  *io = (qbs_io_t){
      .ctx = ctx,
      .read = (qbs_io_read)qbs_stream_read,
      .write = (qbs_io_write)qbs_stream_write,
      .close = (qbs_io_close)qbs_stream_close,
  };
  return 0;
}
