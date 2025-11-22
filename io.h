#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define qbs_io_min(a, b) (((a) < (b)) ? (a) : (b))

typedef enum {
  qbs_io_err_eof = -1,
  qbs_io_err_null = 0,
  qbs_io_err_unexpected_eof = 1,
  qbs_io_err_no_progress = 2,
  qbs_io_err_short_buffer = 3,
  qbs_io_err_long_buffer = 4,
} qbs_io_err;

typedef struct {
  int64_t err;
  uint64_t n;
} qbs_io_respose_t;

typedef qbs_io_respose_t (*qbs_io_read)(void *ctx, uint8_t *bytes, uint64_t size);

typedef qbs_io_respose_t (*qbs_io_write)(void *ctx, uint8_t *bytes, uint64_t size);

typedef uint16_t (*qbs_io_close)(void *ctx);

typedef struct {
  void *ctx;
  qbs_io_read read;
  qbs_io_write write;
  qbs_io_close close;
} qbs_io_t;

typedef struct {
  uint64_t limit;
  uint64_t done;
  qbs_io_t *r;
  bool is_completed;
} qbs_io_limit_t;

const uint8_t *qbs_io_error_to_string(int16_t code) {
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
  assert("unreachable");
  return (uint8_t *)"unreachable";
}

qbs_io_respose_t qbs_io_copy_buffer(qbs_io_t *src, qbs_io_t *dst, uint8_t *buf, uint64_t sz) {
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

qbs_io_respose_t qbs_io_copy(qbs_io_t *src, qbs_io_t *dst) {
  uint8_t mid[512] = {0};
  return qbs_io_copy_buffer(src, dst, mid, sizeof(mid));
}

qbs_io_respose_t qbs_io_limit_read(void *ctx, uint8_t *buf, uint64_t sz) {
  assert(ctx != 0);
  assert(buf != 0);
  assert(sz != 0);

  qbs_io_limit_t *ltx = (qbs_io_limit_t *)ctx;

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

qbs_io_t qbs_io_add_limit(qbs_io_limit_t *l, qbs_io_t *r, uint64_t limit) {
  assert(l != 0);
  assert(r != 0);
  assert(limit != 0);

  *l = (qbs_io_limit_t){
      .limit = limit,
      .done = 0,
      .r = r,
      .is_completed = false,
  };
  return (qbs_io_t){
      .ctx = l,
      .read = qbs_io_limit_read,
      .write = 0,
      .close = 0,
  };
}

qbs_io_respose_t qbs_io_copy_n(qbs_io_t *src, qbs_io_t *dst, uint64_t n) {
  qbs_io_limit_t l = {0};
  qbs_io_t lsrc = qbs_io_add_limit(&l, src, n);
  return qbs_io_copy(&lsrc, dst);
}

qbs_io_respose_t qbs_io_read_at_least(qbs_io_t *r, uint8_t *b, uint64_t sz, uint64_t min) {
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

qbs_io_respose_t qbs_io_read_full(qbs_io_t *r, uint8_t *b, uint64_t sz) { return qbs_io_read_at_least(r, b, sz, sz); }
