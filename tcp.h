#pragma once

#include "io.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <unistd.h>

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

qbs_tcp_listener_t qbs_tcp_listen(char *address, uint16_t port) {
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

qbs_io_tcp_t qbs_tcp_accept(qbs_tcp_listener_t *l) {
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
