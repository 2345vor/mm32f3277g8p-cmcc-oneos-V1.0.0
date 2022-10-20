#ifndef __ONETLS_TLS_H__
#define __ONETLS_TLS_H__
#include "onetls_lib.h"
#include "onetls_alert.h"

uint32_t onetls_negoatiate(onetls_ctx *ctx);
uint32_t onetls_init_state(onetls_ctx *ctx);
uint32_t onetls_send_client_hello(onetls_ctx *ctx);
uint32_t onetls_gen_early_traffic_secret(onetls_ctx *ctx);
uint32_t onetls_send_early_data_process(onetls_ctx *ctx);
uint32_t onetls_recv_server_hello(onetls_ctx *ctx);
uint32_t onetls_tls_recv_server_ee(onetls_ctx *ctx);
uint32_t onetls_tls_handshake_secret(onetls_ctx *ctx);
uint32_t onetls_tls_recv_server_finish(onetls_ctx *ctx);
uint32_t onetls_tls_server_hs_secret(onetls_ctx *ctx);
uint32_t onetls_tls_send_client_ccs(onetls_ctx *ctx);
uint32_t onetls_tls_send_client_eoe(onetls_ctx *ctx);
uint32_t onetls_tls_client_hs_secret(onetls_ctx *ctx);
uint32_t onetls_tls_send_client_finish(onetls_ctx *ctx);
uint32_t onetls_tls_app_secret(onetls_ctx *ctx);
uint32_t onetls_tls_pre_ok(onetls_ctx *ctx);
uint32_t onetls_tls_recv_server_ccs(onetls_ctx *ctx);
uint32_t onetls_deal_with_nst(onetls_ctx* ctx, onetls_buffer *buffer, onetls_resumption *res_data, uint16_t res_len, uint8_t *nonce, uint8_t nonce_len);
uint32_t onetls_tls_recv_key_update(onetls_ctx* ctx, onetls_buffer *buffer);
uint32_t onetls_tls_send_update_key(onetls_ctx* ctx, uint8_t way);
uint32_t onetls_check_dtls_hs_msg(onetls_ctx *ctx);
#endif
