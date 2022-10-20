#ifndef __ONETLS_RECORD_H__
#define __ONETLS_RECORD_H__
#include "onetls_lib.h"
#include "onetls_list.h"
#include "onetls_buffer.h"

// 限制，onetls只接受，并缓存比当前epoch大2个点的报文
#define ONETLS_DTLS_EPOCH_LIMIT 2

#define ONETLS_DTLS_WINDOWS_SIZE (128)

#define ONETLS_DTLS_PACKET_TIMEOUT  (2000)

#define ONETLS_DTLS_WAITING_TIMEOUT (8000)

#define ONETLS_DTLS_HS_MSG_RETRY_TIMES 5

typedef struct {
    uint16_t epoch;
    uint64_t seq_num;
    uint16_t message_seq;       // 握手报文才关注这个字段
    uint8_t type;
    uint32_t time_out;
    onetls_list_node list;

    onetls_buffer *packet;
} onetls_packet_node;

uint32_t onetls_release_cache_list(onetls_handshake *ctx);
uint32_t onetls_send_record(onetls_ctx *ctx, onetls_buffer *buffer);
uint32_t onetls_recv_record(onetls_ctx *ctx, onetls_buffer *buffer);

uint32_t onetls_send_inner(onetls_ctx *ctx, uint8_t *out, uint32_t out_len, uint32_t *send_len);
uint32_t onetls_recv_inner(onetls_ctx *ctx, uint8_t *in, uint32_t in_len, uint32_t *recv_len);

uint32_t onetls_send_encode_data(onetls_ctx *ctx, onetls_buffer *cipher_text, uint8_t *buffer, uint32_t buffer_len, uint8_t mt);

uint32_t onetls_recv_decode_buffer(onetls_ctx *ctx, onetls_buffer *plain_text, onetls_buffer *buffer, uint8_t *mt);
uint32_t onetls_send_encode_buffer(onetls_ctx *ctx, onetls_buffer *cipher_text, onetls_buffer *buffer, uint8_t mt);

uint32_t onetls_want_a_msg(onetls_ctx* ctx, uint8_t mt);

#endif
