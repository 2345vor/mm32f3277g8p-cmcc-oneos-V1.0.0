#include "onetls_lib.h"
#include "onetls_crypto.h"
#include "onetls_alert.h"
#include "onetls_tls.h"
#include "onetls_record.h"
#include <stdio.h>

// 防止重复初始化
static uint8_t g_onetls_init_flag = 0;

static onetls_handshake *onetls_handshake_new(onetls_ctx* ctx)
{
    onetls_handshake *handshake = (onetls_handshake *)onetls_malloc(sizeof(onetls_handshake));
    onetls_assert(handshake != NULL);

    memset(handshake, 0, sizeof(onetls_handshake));

    onetls_list_init(&(handshake->recv_list));
    handshake->recv_list.v_byte = 0;
    return handshake;
}

static void onetls_hanshake_del(onetls_handshake *handshake)
{
    if (handshake == NULL) {
        return;
    }

    onetls_key_share_del(handshake->dh_para.key_share);
    handshake->dh_para.key_share = NULL;

    onetls_buffer_del_safe(&(handshake->dh_para.dh_key));
    onetls_buffer_del_safe(&(handshake->cookie));
    onetls_buffer_del_safe(&(handshake->handshake_packet.handshake_data));

    onetls_release_cache_list(handshake);

    memset(handshake, 0, sizeof(onetls_handshake));
    onetls_free(handshake);
}

static uint32_t onetls_check_data_channel(onetls_ctx *ctx, uint8_t *data, uint32_t data_len, uint32_t *op_len)
{
    *op_len = 0;
    if ((data == NULL) || (data_len == 0)) {
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_check_data_channel buffer");
        return ONETLS_INVALID_PARA;
    }
    // 如果状态不正常了。禁止收发
    if (ctx->state != ONETLS_STATE_OK) {
        onetls_check_errlog(ONETLS_INVALID_STATE, "onetls_check_data_channel state");
        return ONETLS_INVALID_STATE;
    }

    return ONETLS_SUCCESS;
}

// onetls的库初始化
uint32_t onetls_init(void)
{
    uint32_t ret = 0;
    if (g_onetls_init_flag) {
        return ONETLS_SUCCESS;
    }

    ret = onetls_mem_init();
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_mem_init");
        return ret;
    }

    ret = onetls_crypto_init();
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_crypto_init");
        return ret;
    }

    g_onetls_init_flag = 1;
    return ONETLS_SUCCESS;
}

const char *onetls_version(onetls_version_vrcb *vrcb)
{
    static const char *version = ONETLS_NAME" V1R1C00B100, built at "__TIME__" "__DATE__;
    if (vrcb != NULL) {
        vrcb->name = ONETLS_NAME;
        sscanf(version, ONETLS_NAME" V%dR%dC%dB%d", &(vrcb->v), &(vrcb->r), &(vrcb->c), &(vrcb->b));
    }
    return version;
}

uint32_t onetls_is_dtls(onetls_ctx* ctx)
{
    return (ctx->mode == ONETLS_MODE_DTLS);
}

onetls_ctx *onetls_ctx_new(uint8_t mode)
{
    onetls_ctx *ctx = (onetls_ctx *)onetls_malloc(sizeof(onetls_ctx));
    onetls_assert(ctx != NULL);
    memset(ctx, 0, sizeof(onetls_ctx));

    // 初始化成员
    ctx->mode = mode;
    ctx->recv_fd = -1;
    ctx->send_fd = -1;
    ctx->state = ONETLS_STATE_INIT;
    ctx->sub_state = ONETLS_SUB_STATE_PREPARING;
    ctx->mtu = ONETLS_MAX_PACKET_LEN;
    ctx->cipher = onetls_get_ciphers(NULL); // 默认算法套件
    onetls_assert(ctx->cipher != NULL);

    ctx->en_ctx = onetls_crypto_create_ctx();
    onetls_assert(ctx->en_ctx != NULL);

    ctx->de_ctx = onetls_crypto_create_ctx();
    onetls_assert(ctx->de_ctx != NULL);

    ctx->message.msg_out = onetls_buffer_new(ONETLS_MAX_PACKET_LEN + 64);   // 加上记录层头，加上tag/mt，多给32个字节
    onetls_assert(ctx->message.msg_out != NULL);

    ctx->message.msg_in = onetls_buffer_new(ONETLS_MAX_PACKET_LEN + 64);
    onetls_assert(ctx->message.msg_in != NULL);

    return ctx;
}

void onetls_ctx_del(onetls_ctx *ctx)
{
    onetls_shutdown(ctx);

    onetls_buffer_del_safe(&(ctx->message.msg_out));
    onetls_buffer_del_safe(&(ctx->message.msg_in));
    
    onetls_buffer_del_safe(&(ctx->sess_id));
    onetls_buffer_del_safe(&(ctx->server_name));
    onetls_buffer_del_safe(&(ctx->psk_info.hint));
    onetls_buffer_del_safe(&(ctx->psk_info.out_band_psk));
    onetls_buffer_del_safe(&(ctx->psk_info.ticket));
    onetls_buffer_del_safe(&(ctx->early_data.early_buffer));

    onetls_crypto_delete_ctx(ctx->en_ctx);
    ctx->en_ctx = NULL;

    onetls_crypto_delete_ctx(ctx->de_ctx);
    ctx->de_ctx = NULL;
    
    onetls_free(ctx);
}

void onetls_set_socket(onetls_ctx *ctx, int recv_fd, int send_fd)
{
    ctx->recv_fd = recv_fd;
    ctx->send_fd = send_fd;
}

uint32_t onetls_set_outband_psk(onetls_ctx *ctx, uint8_t *hint, uint32_t hint_len, uint8_t *key, uint32_t key_len)
{
    if ((hint == NULL) || (hint_len == 0) || (hint_len > 256) || (key == NULL) || (key_len == 0)) {
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_set_psk_hint para[%d]", hint_len);
        return ONETLS_INVALID_PARA;
    }

    onetls_buffer_del(ctx->psk_info.hint);
    ctx->psk_info.hint = onetls_buffer_new(hint_len);
    onetls_buffer_put_stream(ctx->psk_info.hint, hint, hint_len);

    onetls_buffer_del(ctx->psk_info.out_band_psk);
    ctx->psk_info.out_band_psk = onetls_buffer_new(key_len);
    onetls_buffer_put_stream(ctx->psk_info.out_band_psk, key, key_len);
    return ONETLS_SUCCESS;
}

uint32_t onetls_connect(onetls_ctx *ctx)
{
    uint32_t ret = 0;
    if (ctx->handshake == NULL) {
        ctx->handshake = onetls_handshake_new(ctx);
        onetls_assert(ctx->handshake != NULL);
    }

    ctx->shutdown = 0;  // 不管之前是啥状态，再次连接。不能直接shutdown
    ret = onetls_negoatiate(ctx);
    if (ret != ONETLS_SOCKET_TRYAGAIN) {        
        onetls_hanshake_del(ctx->handshake);
        ctx->handshake = NULL;

        if (ret != ONETLS_SUCCESS) {
            ctx->state = ONETLS_STATE_INIT;
        }
    }
    return ret;
}

void onetls_shutdown_inner(onetls_ctx *ctx)
{
    // 释放握手管理单元
    if (ctx->handshake) {
        onetls_hanshake_del(ctx->handshake);
        ctx->handshake = NULL;
    }
    onetls_buffer_reset(ctx->message.msg_in);
    onetls_buffer_reset(ctx->message.msg_out);
    onetls_buffer_del_safe(&(ctx->message.plain_data));

    ctx->cipher = onetls_get_ciphers(NULL); // 默认算法套件
    onetls_assert(ctx->cipher != NULL);

    memset(&(ctx->security), 0, sizeof(ctx->security));

    ctx->state = ONETLS_STATE_INIT;
}

void onetls_shutdown(onetls_ctx *ctx)
{
    if (ctx->state != ONETLS_STATE_INIT) {
        onetls_notify_close(ctx);
    }
    onetls_shutdown_inner(ctx);
}

uint32_t onetls_recv(onetls_ctx *ctx, uint8_t *in, uint32_t in_len, uint32_t *recv_len)
{
    return onetls_check_data_channel(ctx, in, in_len, recv_len) ? 
           ONETLS_FAIL : 
           onetls_recv_inner(ctx, in, in_len, recv_len);
}

uint32_t onetls_send(onetls_ctx *ctx, uint8_t *out, uint32_t out_len, uint32_t *send_len)
{
    return onetls_check_data_channel(ctx, out, out_len, send_len) ? 
           ONETLS_FAIL : 
           onetls_send_inner(ctx, out, out_len, send_len);
}

uint32_t onetls_send_early_data(onetls_ctx *ctx, uint8_t *out, uint32_t out_len)
{
    if ((out == NULL) || (out_len == 0)) {
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_send_early_data para");
        return ONETLS_INVALID_PARA;
    }

    if (ctx->state >= ONETLS_STATE_SEND_EARLY_DATA) {
        onetls_check_errlog(ONETLS_INVALID_STATE, "onetls_send_early_data state");
        return ONETLS_INVALID_STATE;
    }

    if ((ctx->early_data.max_early_data != 0) && (out_len > ctx->early_data.max_early_data)) {
        onetls_check_errlog(ONETLS_TOO_MUCH_EARLY_DATA, "onetls_send_early_data too much data");
        return ONETLS_TOO_MUCH_EARLY_DATA;
    }

    onetls_buffer_put_stream_realloc(&(ctx->early_data.early_buffer), out, out_len, 0);
    onetls_buffer_set_cursor(ctx->early_data.early_buffer, 0);
    return ONETLS_SUCCESS;
}

uint32_t onetls_flush(onetls_ctx *ctx)
{
    uint32_t ret = ONETLS_SOCKET_TRYAGAIN;
    if (onetls_buffer_cursor_to_tail(ctx->message.msg_out) == 0) {
        return ONETLS_SUCCESS;
    }
    while (ret == ONETLS_SOCKET_TRYAGAIN) {
        ret = onetls_send_record(ctx, ctx->message.msg_out);
    }
    return ret;
}

uint32_t onetls_pending(onetls_ctx *ctx)
{
    if (ctx->message.plain_data != NULL) {
        return onetls_buffer_cursor_to_tail(ctx->message.plain_data);
    }
    return 0;
}

void onetls_set_recv_ticket_cb(onetls_ctx *ctx, onetls_recv_nst_callback cb)
{
    ctx->resumption.ticket_cb = cb;
}

uint32_t onetls_set_ticket(onetls_ctx *ctx, void *data, uint32_t data_len)
{
    onetls_resumption *res_data = (onetls_resumption *)data;
    if ((res_data == NULL) || (data_len == 0) || (res_data->ticket_len == 0)) {
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_set_ticket para");
        return ONETLS_INVALID_PARA;
    }
    
    // TODO: 
    // 当前设备没有时间函数，暂时不校验有效期了
    // data的合法性校验也需要考虑，当前暂时不做，放在V2.0做
    ctx->early_data.max_early_data = res_data->max_early_data;
    ctx->cipher = onetls_get_cipher_byid(res_data->cipher_id);

    ctx->psk_info.ticket = onetls_buffer_new(res_data->ticket_len);
    onetls_assert(ctx->psk_info.ticket != NULL);
    onetls_buffer_put_stream(ctx->psk_info.ticket, res_data->ticket, res_data->ticket_len); 

    memcpy(ctx->security.master_key, res_data->master_key, ONETLS_MAX_MD_LEN);
    return ONETLS_SUCCESS;
}

uint32_t onetls_update_key(onetls_ctx *ctx, uint8_t way)
{
    if (ctx->state != ONETLS_STATE_OK) {
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_update_key[%d]", ctx->state);
        return ONETLS_INVALID_STATE;
    }
    if (way > ONETLS_KEY_UPDATE_REQUEST) {
        return ONETLS_INVALID_PARA;
    }
    return onetls_tls_send_update_key(ctx, way);
}

uint32_t onetls_set_mtu(onetls_ctx *ctx, uint16_t mtu)
{
    if (mtu < 384) {    // 最小的mtu不能低于384
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_set_mtu, can not lower than 384!");
        return ONETLS_INVALID_PARA;
    }
    ctx->mtu = onetls_min(mtu, ONETLS_MAX_PACKET_LEN);
    return ONETLS_SUCCESS;
}