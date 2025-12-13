#pragma once

#include "io.h"
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

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

inline qbs_io_respose_t qbs_file_close(qbs_file_ctx_t *ctx) {
  return (qbs_io_respose_t){
      .err = close(ctx->fd),
  };
}

inline qbs_io_respose_t qbs_file_read(qbs_file_ctx_t *ctx, uint8_t *b, uint64_t sz) {
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

inline qbs_io_respose_t qbs_file_write(qbs_file_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(ctx != 0);
  assert(b != 0);
  assert(ctx->mode & O_WRONLY || ctx->mode & O_RDWR);

  uint64_t res = write(ctx->fd, b, sz);
  return (qbs_io_respose_t){
      .err = res < 0 ? (int64_t)res : qbs_io_err_null,
      .n = res,
  };
}

inline qbs_io_file_t qbs_file_open(char *filename, int mode) {
  int fd = open(filename, mode, 0644);
  if (fd < 0)
    return (qbs_io_file_t){.err = fd};

  qbs_io_read rcb = (mode & O_RDWR || mode & O_RDONLY) ? (qbs_io_read)qbs_file_read : qbs_io_invalid_rw;
  qbs_io_write wcb = (mode & O_RDWR || mode & O_WRONLY) ? (qbs_io_write)qbs_file_write : qbs_io_invalid_rw;

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
