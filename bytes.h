#pragma once

#include "io.h"
#include <assert.h>
#include <stdint.h>
#include <unistd.h>

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

inline qbs_io_respose_t qbs_bytes_read(qbs_bytes_ctx_t *ctx, uint8_t *b, uint64_t sz) {
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

inline qbs_io_respose_t qbs_bytes_write(qbs_bytes_ctx_t *ctx, uint8_t *b, uint64_t sz) {
  assert(sz <= ctx->capacity - ctx->offset);

  for (size_t i = 0; i < sz; i++)
    ctx->buffer[ctx->offset++] = b[i];

  return (qbs_io_respose_t){
      .err = qbs_io_err_null,
      .n = sz,
  };
}

inline qbs_bytes_io_t qbs_bytes_reader(uint8_t *buffer, uint64_t size) {
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

inline qbs_bytes_io_t qbs_bytes_writer(uint8_t *buffer, uint64_t size) {
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
