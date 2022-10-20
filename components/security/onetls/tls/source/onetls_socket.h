#ifndef __ONETLS_SOCKET_H__
#define __ONETLS_SOCKET_H__
#include "onetls_client.h"
#include "onetls_buffer.h"

// 通用的记录层收发长度
#define ONETLS_ADDITIONAL_CIPHER_LEN (32)
#define ONETLS_RECORD_HEADER_LEN (onetls_is_dtls(ctx) ? (13) : (5))

// 探测接口
#define ONETLS_SOCK_RD (0)
#define ONETLS_SOCK_WR (1)
uint32_t onetls_sock_test(int fd, uint8_t op, uint32_t timeout);

uint32_t onetls_sock_recv(onetls_ctx *ctx, uint8_t *buf, uint32_t len, uint32_t *recv_len, uint32_t time_out);
uint32_t onetls_sock_send(onetls_ctx *ctx, uint8_t *buf, uint32_t len, uint32_t *send_len, uint32_t time_out);

#endif
