#include "onetls_tls.h"
#include "onetls_record.h"
#include "onetls_crypto.h"
#include "onetls_hkdf.h"
#include "onetls_extension.h"

#define MESSAGE_HASH_HEADER_LENGTH 4

static uint8_t g_onetls_hello_retry_req[32] = {
    0xCF, 0x21, 0xAD, 0x74, 0xE5, 0x9A, 0x61, 0x11, 
    0xBE, 0x1D, 0x8C, 0x02, 0x1E, 0x65, 0xB8, 0x91,
    0xC2, 0xA2, 0x11, 0x16, 0x7A, 0xBB, 0x8C, 0x5E, 
    0x07, 0x9E, 0x09, 0xE2, 0xC8, 0xA8, 0x33, 0x9C,
};

static onetls_state_func g_onetls_state_machine[ONETLS_STATE_OK] = {
    // 准备活动主要是校验、获取PSK等检查操作
    [ONETLS_STATE_INIT] =                       onetls_init_state,
    // 发送client_hello
    [ONETLS_STATE_SEND_CNT_HELLO] =             onetls_send_client_hello,
    // 构造early traffic secret
    [ONETLS_STATE_GEN_C_EARLY_TRAFFIC_SECRET] = onetls_gen_early_traffic_secret,
    // 发送early_data
    [ONETLS_STATE_SEND_EARLY_DATA] =            onetls_send_early_data_process,
    // 收到server_hello
    [ONETLS_STATE_RECV_SVR_HELLO] =             onetls_recv_server_hello,
    // 派生handshake secret
    [ONETLS_STATE_GEN_HS_SECRET] =              onetls_tls_handshake_secret,
    // 派生server handshake secret
    [ONETLS_STATE_GEN_S_HS_SECRET] =            onetls_tls_server_hs_secret,
    // 收到server的ee报文
    [ONETLS_STATE_RECV_SVR_EE] =                onetls_tls_recv_server_ee,
    // 收到server的finish报文
    [ONETLS_STATE_RECV_SVR_FINISH] =            onetls_tls_recv_server_finish,
    // 发送client ccs报文，保留该流程
    [ONETLS_STATE_SEND_CNT_CCS] =               onetls_tls_send_client_ccs,
    // 发送client的end of early_dta
    [ONETLS_STATE_SEND_EOE] =                   onetls_tls_send_client_eoe, 
    // 派生server handshake secret
    [ONETLS_STATE_GEN_C_HS_SECRET] =            onetls_tls_client_hs_secret,
    // 发送client报文
    [ONETLS_STATE_SEND_CNT_FINISH] =            onetls_tls_send_client_finish,
    // 派生app秘钥
    [ONETLS_STATE_GEN_APP_SECRET] =             onetls_tls_app_secret,
    // 握手完成，准备清理中间信息
    [ONETLS_STATE_PRE_OK] =                     onetls_tls_pre_ok,    
};

static uint32_t onetls_gen_master_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_secret(ctx, 
                                     NULL, 0,
                                     ctx->handshake->secret.handshake_secret, 
                                     ctx->security.master_secret);
    onetls_log_dump("master secret", ctx->security.master_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    return ret;
}

static uint32_t onetls_gen_res_master_key(onetls_ctx* ctx, onetls_resumption *res_data, uint8_t *nonce, uint8_t nonce_len)
{
    static const uint8_t label[] = "resumption";
    uint32_t ret = onetls_hkdf_expand_label(ctx, 
                                            ctx->security.resume_master_secret, onetls_hash_size(ctx->cipher->hkdf_hash), 
                                            label, sizeof(label) - 1, 
                                            nonce, nonce_len, 
                                            res_data->master_key, onetls_hash_size(ctx->cipher->hkdf_hash));
    onetls_check_errlog(ret, "onetls_gen_res_master_key onetls_hkdf_expand_label");
    onetls_log_dump("resump_master_key", res_data->master_key, onetls_hash_size(ctx->cipher->hkdf_hash));
    return ret;
}

static uint32_t onetls_gen_c_app_traffic_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_traffic_secret_key_iv(ctx, 
                                                    "c ap traffic", 
                                                    ctx->handshake->handshake_packet.ch_to_sf, onetls_hash_size(ctx->cipher->hkdf_hash),
                                                    ctx->security.master_secret, 
                                                    ctx->security.c_app_secret, 
                                                    ctx->security.wr_key, ctx->cipher->key_len,
                                                    ctx->security.wr_iv, ctx->cipher->iv_len,
                                                    NULL,
                                                    ctx->security.c_sn_key); 
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_traffic_secret_key_iv c app");
        return ret;
    }
    onetls_log_dump("c_app_traffic", ctx->security.c_app_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    if (onetls_is_dtls(ctx)) {
        ctx->security.dtls.record_num_wr.epoch = ONETLS_EPOCH_FOR_APP_DATA;
        onetls_seq_num_reset(ctx->security.dtls.record_num_wr.seq_num, 6);
    }
    onetls_seq_num_reset(ctx->security.wr_seq, 8);
    return onetls_crypto_ctx_init(ctx->en_ctx, ctx->cipher, ctx->security.wr_key, 1);
}

static uint32_t onetls_gen_s_app_traffic_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_traffic_secret_key_iv(ctx, 
                                                    "s ap traffic", 
                                                    ctx->handshake->handshake_packet.ch_to_sf, onetls_hash_size(ctx->cipher->hkdf_hash),
                                                    ctx->security.master_secret, 
                                                    ctx->security.s_app_secret, 
                                                    ctx->security.rd_key, ctx->cipher->key_len,
                                                    ctx->security.rd_iv, ctx->cipher->iv_len,
                                                    NULL,
                                                    ctx->security.s_sn_key); 
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_traffic_secret_key_iv s app");
        return ret;
    }
    onetls_log_dump("s_app_traffic", ctx->security.s_app_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    if (onetls_is_dtls(ctx)) {
        ctx->security.dtls.record_num_rd.epoch = ONETLS_EPOCH_FOR_APP_DATA;
        onetls_seq_num_reset(ctx->security.dtls.record_num_rd.seq_num, 6);
    }
    onetls_seq_num_reset(ctx->security.rd_seq, 8);
    return onetls_crypto_ctx_init(ctx->de_ctx, ctx->cipher, ctx->security.rd_key, 0);
}

static uint32_t onetls_gen_res_master_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_traffic_secret_key_iv(ctx, 
                                                    "res master", 
                                                    ctx->handshake->handshake_packet.ch_to_cf, onetls_hash_size(ctx->cipher->hkdf_hash),
                                                    ctx->security.master_secret, 
                                                    ctx->security.resume_master_secret,
                                                    NULL, 0, NULL, 0, NULL, NULL); 
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_res_master_secret");
        return ret;
    }
    onetls_log_dump("resump_master_secret", ctx->security.resume_master_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    return ONETLS_SUCCESS;
} 

static uint32_t onetls_gen_c_hs_traffic_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_traffic_secret_key_iv(ctx, 
                                                    "c hs traffic", 
                                                    ctx->handshake->handshake_packet.ch_to_sh, onetls_hash_size(ctx->cipher->hkdf_hash),
                                                    ctx->handshake->secret.handshake_secret, 
                                                    ctx->handshake->secret.client_handshake_secret, 
                                                    ctx->security.wr_key, ctx->cipher->key_len,
                                                    ctx->security.wr_iv, ctx->cipher->iv_len,
                                                    ctx->handshake->secret.c_finish_key,
                                                    ctx->security.c_sn_key); 
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_traffic_secret_key_iv c hs");
        return ret;
    }
    onetls_log_dump("c_hs_traffic", ctx->handshake->secret.client_handshake_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    if (onetls_is_dtls(ctx)) {
        ctx->security.dtls.record_num_wr.epoch = ONETLS_EPOCH_FOR_HS_MSG;
        onetls_seq_num_reset(ctx->security.dtls.record_num_wr.seq_num, 6);
    }    
    onetls_seq_num_reset(ctx->security.wr_seq, 8);
    return onetls_crypto_ctx_init(ctx->en_ctx, ctx->cipher, ctx->security.wr_key, 1);
}

static uint32_t onetls_gen_s_hs_traffic_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_traffic_secret_key_iv(ctx, 
                                                    "s hs traffic", 
                                                    ctx->handshake->handshake_packet.ch_to_sh, onetls_hash_size(ctx->cipher->hkdf_hash),
                                                    ctx->handshake->secret.handshake_secret, 
                                                    ctx->handshake->secret.server_handshake_secret, 
                                                    ctx->security.rd_key, ctx->cipher->key_len,
                                                    ctx->security.rd_iv, ctx->cipher->iv_len,
                                                    ctx->handshake->secret.s_finish_key,
                                                    ctx->security.s_sn_key); 
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_traffic_secret_key_iv s hs");
        return ret;
    }
    onetls_log_dump("s_hs_traffic", ctx->handshake->secret.server_handshake_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    if (onetls_is_dtls(ctx)) {
        ctx->security.dtls.record_num_rd.epoch = ONETLS_EPOCH_FOR_HS_MSG;
        onetls_seq_num_reset(ctx->security.dtls.record_num_rd.seq_num, 6);
    }    
    onetls_seq_num_reset(ctx->security.rd_seq, 8);
    return onetls_crypto_ctx_init(ctx->de_ctx, ctx->cipher, ctx->security.rd_key, 0);
}

static uint32_t onetls_gen_hs_traffic_secret(onetls_ctx *ctx)
{
    uint8_t *dh_key = NULL;
    uint32_t dh_key_len = 0;
    if (ctx->handshake->dh_para.dh_key) {
        dh_key = onetls_buffer_get_data(ctx->handshake->dh_para.dh_key);
        dh_key_len = onetls_buffer_get_tail(ctx->handshake->dh_para.dh_key);
    }

    uint32_t ret = onetls_gen_secret(ctx, 
                                     dh_key, dh_key_len,
                                     ctx->handshake->secret.early_secret[ctx->handshake->secret.select_psk],
                                     ctx->handshake->secret.handshake_secret);
    onetls_log_dump("handshake_secret", ctx->handshake->secret.handshake_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    return ret;
}

static uint32_t onetls_update_traffic_key(onetls_ctx *ctx, uint8_t sending)
{
    uint32_t ret = 0;
    uint8_t *iv = NULL, *key = NULL, *secret_n = NULL, *seq = NULL, *sn_key;
    uint8_t out[ONETLS_MAX_MD_LEN] = { 0 };
    void *crypto_ctx = NULL;
    if (sending) {
        iv = ctx->security.wr_iv;
        key = ctx->security.wr_key;
        secret_n = ctx->security.c_app_secret;
        crypto_ctx = ctx->en_ctx;
        seq = ctx->security.wr_seq;
        sn_key = ctx->security.c_sn_key;
    } else {
        iv = ctx->security.rd_iv;
        key = ctx->security.rd_key;
        secret_n = ctx->security.s_app_secret;
        crypto_ctx = ctx->de_ctx;
        seq = ctx->security.rd_seq;
        sn_key = ctx->security.s_sn_key;
    }

    ret = onetls_gen_traffic_secret_key_iv(ctx, 
                                           "traffic upd", NULL, 0,
                                           secret_n, out,
                                           key, ctx->cipher->key_len,
                                           iv, ctx->cipher->iv_len,
                                           NULL,
                                           sn_key); 
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_traffic_secret_key_iv");
        return ret;
    }

    onetls_log_dump("update traffic", out, onetls_hash_size(ctx->cipher->hkdf_hash));
    onetls_seq_num_reset(seq, 8);
    memcpy(secret_n, out, ONETLS_MAX_MD_LEN);
    return onetls_crypto_ctx_init(crypto_ctx, ctx->cipher, key, sending);
}

static uint32_t onetls_tls_pkt_update_key(onetls_ctx* ctx, uint8_t way, onetls_buffer *buffer)
{
    uint32_t ret = 0;
    uint8_t pkt[ONETLS_TLS_KEY_UPDATE_LEN] = { 0 };

    pkt[0] = ONETLS_KEY_UPDATE;
    pkt[1] = 0;
    pkt[2] = 0;
    pkt[3] = 1;
    pkt[4] = way;

    // 加密
    onetls_buffer_reset(ctx->message.msg_out);
    ret = onetls_send_encode_data(ctx, buffer, pkt, ONETLS_TLS_KEY_UPDATE_LEN, ONETLS_MT_HANDSHAKE);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;   
    }
    return ONETLS_SUCCESS;
}

static void onetls_check_record_timeout(onetls_ctx *ctx)
{
    onetls_list_node *next_node = NULL;    
    onetls_list_node *cur_node = ctx->handshake->recv_list.next;
    while (cur_node != &(ctx->handshake->recv_list)) {
        next_node = cur_node->next;

        onetls_packet_node *packet_node = (onetls_packet_node *)(cur_node->v_ptr);
        onetls_assert(packet_node != NULL);
        if ((packet_node->time_out + ONETLS_DTLS_PACKET_TIMEOUT) < onetls_get_time()) {
            onetls_list_del(cur_node);
            onetls_buffer_del(packet_node->packet);
            onetls_free(packet_node);
            ctx->handshake->recv_list.v_byte --;
        } 
        cur_node = next_node;
    }
}

static uint32_t onetls_check_record(onetls_ctx *ctx) 
{
    if (!onetls_is_dtls(ctx)) {
        return ONETLS_SUCCESS;  // TLS不用处理记录层报文
    }
    if (ctx->sub_state == ONETLS_SUB_STATE_PREPARING) {
        return ONETLS_SUCCESS;
    }
    if ((ctx->state >= ONETLS_STATE_OK) || (ctx->handshake == NULL)) {
        return ONETLS_SUCCESS;
    }
    // 删除超时的分片
    onetls_check_record_timeout(ctx);

    // 判断是否存在需要的分片
    return onetls_check_dtls_hs_msg(ctx);
}

static uint32_t onetls_put_binder(onetls_ctx *ctx, uint8_t *secret, onetls_buffer *buffer, const char *label, uint8_t *digest, uint32_t digest_len)
{
    uint32_t ret = 0, md_size = onetls_hash_size(ctx->cipher->hkdf_hash);
    uint8_t binder_key[ONETLS_MAX_MD_LEN] = { 0 }, finish_key[ONETLS_MAX_MD_LEN] = { 0 }, hash[ONETLS_MAX_MD_LEN] = { 0 };
    
    ret = onetls_hkdf_derive_secret(ctx, (const uint8_t *)label, strlen(label), secret, md_size, NULL, 0, binder_key, md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_derive_secret");
        return ret;
    }
    onetls_log_dump(label, binder_key, md_size);

    ret = onetls_hkdf_derive_finished(ctx, binder_key, md_size, finish_key, md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_derive_finished");
        return ret;
    }

    ret = onetls_hmac(ctx->cipher->hkdf_hash, finish_key, md_size, digest, digest_len, hash, &md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hmac");
        return ret;
    }
    onetls_buffer_put_uint8(buffer, md_size);
    onetls_buffer_put_stream(buffer, hash, md_size);
    return ret;
}

static uint32_t onetls_calculate_binder(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t ret = 0, md_size = onetls_hash_size(ctx->cipher->hkdf_hash);
    uint16_t msg_len = 0, binder_len = 0;
    uint8_t hash[ONETLS_MAX_MD_LEN] = { 0 }, *msg = NULL;

    binder_len += (ctx->psk_info.ticket != NULL) ? (md_size + 1) : 0;
    binder_len += (ctx->psk_info.hint != NULL)   ? (md_size + 1) : 0;

    // 这就是除了psk扩展的整个报文的长度，用于计算binder
    msg = onetls_buffer_get_data(buffer) + ONETLS_RECORD_HEADER_LEN;
    msg_len = onetls_buffer_get_tail(buffer) - ONETLS_RECORD_HEADER_LEN - binder_len - 2;

    // 计算消息的摘要
    ret = onetls_hash(ctx->cipher->hkdf_hash, msg, msg_len, hash, &md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        return ret;
    }

    onetls_buffer_set_cursor(buffer, onetls_buffer_get_tail(buffer) - binder_len);
    if (ctx->psk_info.ticket != NULL) {
        ret = onetls_put_binder(ctx, ctx->handshake->secret.early_secret[1], buffer, "res binder", hash, md_size);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_put_binder ticket");
            return ret;
        }
    }

    if (ctx->psk_info.hint != NULL) {
        ret = onetls_put_binder(ctx, ctx->handshake->secret.early_secret[0], buffer, "ext binder", hash, md_size);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_put_binder hint");
            return ret;
        }
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_construct_client_hello_ex(onetls_ctx *ctx)
{
    uint32_t ret = 0, num = 0, offset = 0, len = 0;
    const onetls_cipher *ciphers = onetls_get_ciphers(&num);    
    onetls_buffer *buffer = ctx->message.msg_out;
    onetls_buffer_put_stream(buffer, ctx->handshake->random_c, sizeof(ctx->handshake->random_c));
    // session 长度
    if (ctx->sess_id == NULL) {
        onetls_buffer_put_uint8(buffer, 0);
    } else {
        onetls_buffer_put_uint8(buffer, onetls_buffer_get_tail(ctx->sess_id));
        onetls_buffer_put_stream(buffer, onetls_buffer_get_data(ctx->sess_id), onetls_buffer_get_tail(ctx->sess_id));
    }

    if (onetls_is_dtls(ctx)) {
        onetls_buffer_put_uint8(buffer, 0); // cookie，现在都走扩展了。用不到了
    }

    onetls_buffer_put_uint16(buffer, num * sizeof(uint16_t));
    for (offset = 0; offset < num; offset++) {
        onetls_buffer_put_uint16(buffer, ciphers[offset].iana_id);
    }
    onetls_buffer_put_uint16(buffer, 0x0100);

    // 添加扩展
    ret = onetls_construct_client_hello_extension(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_construct_hello_ext");
        return ret;
    }

    offset = ONETLS_RECORD_HEADER_LEN + 1;
    onetls_buffer_set_cursor(buffer, offset);

    len = onetls_buffer_get_tail(buffer) - offset - 3;
    if (onetls_is_dtls(ctx)) {
        len -= 8;
        onetls_buffer_put_uint24(buffer, len);
        onetls_buffer_put_uint16(buffer, (ctx->handshake->msg_tag.message_seq_wr)++);
        onetls_buffer_put_uint24(buffer, 0);    // 未分片，不支持client hello分片
        onetls_buffer_put_uint24(buffer, len);  // 当前分片长度
    } else {
        onetls_buffer_put_uint24(buffer, len);
    }
    
    // 现在可以开始计算binder了。
    return onetls_calculate_binder(ctx, buffer);
}

static uint32_t onetls_construct_client_hello_packet(onetls_ctx *ctx) 
{
    onetls_buffer *buffer = ctx->message.msg_out;
    uint32_t ret = 0;

    // 预留记录层头
    onetls_buffer_setup_cursor(buffer, ONETLS_RECORD_HEADER_LEN);
    onetls_buffer_put_uint8(buffer, ONETLS_CLIENT_HELLO);
    onetls_buffer_put_uint24(buffer, 0);    // 长度区域预留，等填充完了，再来反写
    if (onetls_is_dtls(ctx)) {
        onetls_buffer_put_uint16(buffer, 0);
        onetls_buffer_put_uint48(buffer, 0);    
        onetls_buffer_put_uint16(buffer, ONETLS_VERSION_DTLS_12);
    } else {
        onetls_buffer_put_uint16(buffer, ONETLS_VERSION_TLS_12);
    }

    ret = onetls_construct_client_hello_ex(ctx); // 构造报文
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_construct_client_hello_ex");
        return ret;
    }

    onetls_buffer_set_cursor(ctx->message.msg_out, 0); // 放置到头部
    onetls_buffer_put_uint8(ctx->message.msg_out, ONETLS_MT_HANDSHAKE);
    if (onetls_is_dtls(ctx)) {
        onetls_buffer_put_uint16(ctx->message.msg_out, ONETLS_VERSION_DTLS_12);
        onetls_buffer_put_uint16(ctx->message.msg_out, ctx->security.dtls.record_num_wr.epoch);
        onetls_buffer_put_stream(ctx->message.msg_out, ctx->security.dtls.record_num_wr.seq_num, 6);
        onetls_seq_num_add(ctx->security.dtls.record_num_wr.seq_num, 6);
    } else {
        onetls_buffer_put_uint16(ctx->message.msg_out, ONETLS_VERSION_TLS_12);
    }
    onetls_buffer_put_uint16(ctx->message.msg_out, onetls_buffer_get_tail(ctx->message.msg_out) - ONETLS_RECORD_HEADER_LEN);

    onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), 
                                     onetls_buffer_get_data(ctx->message.msg_out) + ONETLS_RECORD_HEADER_LEN, 
                                     onetls_buffer_get_tail(ctx->message.msg_out) - ONETLS_RECORD_HEADER_LEN,
                                     512); // 预留512字节给后续的协商报文，免得反复申请内存，可以优化
    return ONETLS_SUCCESS;
}

static uint32_t onetls_gen_early_traffic_secret_ex(onetls_ctx *ctx)
{
    uint8_t *in_sec = onetls_check_data_empty(ctx->handshake->secret.early_secret[1], onetls_hash_size(ctx->cipher->hkdf_hash)) ? 
                      ctx->handshake->secret.early_secret[0] : 
                      ctx->handshake->secret.early_secret[1];
    uint32_t ret = onetls_gen_traffic_secret_key_iv(ctx, 
                                                    "c e traffic", 
                                                    ctx->handshake->handshake_packet.ch, onetls_hash_size(ctx->cipher->hkdf_hash),
                                                    in_sec, 
                                                    ctx->handshake->secret.early_traffic_secret,
                                                    ctx->security.wr_key, ctx->cipher->key_len,
                                                    ctx->security.wr_iv, ctx->cipher->iv_len,
                                                    NULL,
                                                    ctx->security.c_sn_key);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_traffic_secret_key_iv c e");
        return ret;
    }
    onetls_log_dump("c_e_traffic", ctx->handshake->secret.early_traffic_secret, onetls_hash_size(ctx->cipher->hkdf_hash));
    if (onetls_is_dtls(ctx)) {
        ctx->security.dtls.record_num_wr.epoch = ONETLS_EPOCH_FOR_EARLY_DATA;
        onetls_seq_num_reset(ctx->security.dtls.record_num_wr.seq_num, 6);
    }    
    onetls_seq_num_reset(ctx->security.wr_seq, 8);
    return onetls_crypto_ctx_init(ctx->en_ctx, ctx->cipher, ctx->security.wr_key, 1);
}

static void onetls_parse_srv_random(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint8_t hdr[MESSAGE_HASH_HEADER_LENGTH] = { 0 };
    onetls_buffer_get_stream(buffer, ctx->handshake->random_s, 32);

    // TODO: 判断retry
    if (memcmp(g_onetls_hello_retry_req, ctx->handshake->random_s, 32) == 0) {
        onetls_buffer_reset(ctx->handshake->handshake_packet.handshake_data);
        hdr[0] = ONETLS_MESSAGE_HASH;
        hdr[3] = onetls_hash_size(ctx->cipher->hkdf_hash);

        onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), hdr, MESSAGE_HASH_HEADER_LENGTH, 0);
        onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), 
                                         ctx->handshake->handshake_packet.ch, onetls_hash_size(ctx->cipher->hkdf_hash), 0);
        ctx->handshake->msg_tag.server_hello_retry ++;
    } else {
        ctx->handshake->msg_tag.server_hello_retry = 0;
    }
}

static uint32_t onetls_tls_parse_srv_session(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint8_t sess_id_len = 0;
    onetls_buffer_get_uint8(buffer, &sess_id_len);
    if (sess_id_len > onetls_buffer_cursor_to_tail(buffer)) {
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_tls_parse_srv_session");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_RECORD_OVERFLOW);
        return ONETLS_INVALID_PARA;
    }

    if (sess_id_len > 0) {  // 解析session id，会话恢复成功了
        onetls_buffer_del(ctx->sess_id);
        ctx->sess_id = onetls_buffer_new(sess_id_len);
        onetls_assert(ctx->sess_id != NULL);

        // TODO: 这里其实要根据带回来的session id来判断是否是回话恢复，先不考虑，后续要补充
        onetls_buffer_get_stream(buffer, onetls_buffer_get_data(ctx->sess_id), sess_id_len);
        onetls_buffer_setup_tail(ctx->sess_id, sess_id_len);
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_parse_srv_cipher(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint16_t cipher_id = 0;
    const onetls_cipher *cipher = NULL;
    // 校验算法套件
    onetls_buffer_get_uint16(buffer, &cipher_id);
    cipher = onetls_get_cipher_byid(cipher_id);
    if (cipher == NULL) {
        onetls_check_errlog(ONETLS_NO_CIPHER_MATCH, "unkown cipher");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_NO_CIPHER_MATCH;
    }
    // 算法替换
    ctx->cipher = cipher;
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_parse_srv_hello_head_ex(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint8_t comp = 0;    
    uint32_t ret = onetls_tls_parse_srv_session(ctx, buffer);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_parse_srv_session");
        return ret;
    }

    ret = onetls_tls_parse_srv_cipher(ctx, buffer);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_parse_srv_cipher");
        return ret;
    }

    // 压缩算法，不支持了。
    onetls_buffer_get_uint8(buffer, &comp);
    if (comp != 0) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXCEPTION_MSG;
    }
    return ret;
}

static uint32_t onetls_parse_server_hello_head(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t len = 0, payload_len = 0;
    uint16_t version = 0, legacy_version = onetls_is_dtls(ctx) ? ONETLS_VERSION_DTLS_12 : ONETLS_VERSION_TLS_12;
    uint16_t message_seq = 0;
    uint32_t fragment_offset = 0, fragment_length = 0;

    onetls_buffer_get_uint24(buffer, &len);
    if (onetls_is_dtls(ctx)) {
        onetls_buffer_get_uint16(buffer, &message_seq);
        onetls_buffer_get_uint24(buffer, &fragment_offset);
        onetls_buffer_get_uint24(buffer, &fragment_length);
        if ((fragment_offset != 0) || (len != fragment_length)) { // 暂不支持分片
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_BAD_RECORD_MAC);
            return ONETLS_INVALID_PARA;
        }
        if (message_seq != ctx->handshake->msg_tag.message_seq_rd) {
            return ONETLS_INVALID_PARA;
        }
        ctx->handshake->msg_tag.message_seq_rd ++;
    }
    onetls_buffer_get_uint16(buffer, &version);
    // 判断版本号
    if (version < legacy_version) {
        onetls_check_errlog(ONETLS_WRONG_VERSION, "unsupport version");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_PROTOCOL_VERSION);
        return ONETLS_WRONG_VERSION;
    }

    payload_len = onetls_buffer_get_tail(buffer) - ONETLS_RECORD_HEADER_LEN - 4;
    if (onetls_is_dtls(ctx)) {
        payload_len -= 8;   // 握手报文里面的message_seq不算负载
    }
    if ((onetls_buffer_get_tail(buffer) < ONETLS_HANSHAKE_SH_MIN_LEN) || (len != payload_len)) {
        onetls_check_errlog(ONETLS_WRONG_VERSION, "server hello wrong len");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_BAD_RECORD_MAC);
        return ONETLS_RECORD_INVALID_LEN;
    }

    onetls_parse_srv_random(ctx, buffer);
    if (ctx->handshake->msg_tag.server_hello_retry > 1) {   // 重试一次就好了。不要一直来
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_WRONG_RANDWOM_NUM;
    }
    return onetls_tls_parse_srv_hello_head_ex(ctx, buffer);
}

static uint32_t onetls_parse_server_hello(onetls_ctx *ctx, onetls_buffer *buffer)
{
    uint32_t ret = onetls_parse_server_hello_head(ctx, buffer);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_parse_server_hello_head");
        return ret;
    }

    ret = onetls_tls_parse_server_extension(ctx, buffer, ONETLS_STATE_RECV_SVR_HELLO);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_parse_server_ext");
        return ret;
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_tls_parse_server_ee(onetls_ctx *ctx, onetls_buffer *plain_buffer)
{
    uint32_t ret = 0;
    uint16_t message_seq = 0;
    uint32_t t_len = 0, fragment_offset = 0, fragment_length = 0;
    uint8_t mt = 0;
    onetls_buffer_get_uint8(plain_buffer, &mt);
    if (mt != ONETLS_ENCRYPTED_EXTENSIONS) {   // 报文不对
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_UNEXPECTED_MESSAGE);
        return ret;        
    }
    onetls_buffer_get_uint24(plain_buffer, &t_len);
    if (onetls_is_dtls(ctx)) {
        onetls_buffer_get_uint16(plain_buffer, &message_seq);
        onetls_buffer_get_uint24(plain_buffer, &fragment_offset);
        onetls_buffer_get_uint24(plain_buffer, &fragment_length);
        if ((fragment_offset != 0) || (t_len != fragment_length)) { // 暂不支持分片
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_BAD_RECORD_MAC);
            return ONETLS_INVALID_PARA;
        }
        if (message_seq != ctx->handshake->msg_tag.message_seq_rd) {
            return ONETLS_INVALID_PARA;
        }
        ctx->handshake->msg_tag.message_seq_rd ++;
    }

    if (t_len != onetls_buffer_cursor_to_tail(plain_buffer)) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_BAD_RECORD_MAC);
        return ONETLS_INVALID_PARA;
    }

    ret = onetls_tls_parse_server_extension(ctx, plain_buffer, ONETLS_STATE_RECV_SVR_EE);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_parse_hello_ext");
        return ret;
    }
    return ret;
}

uint32_t onetls_init_state(onetls_ctx *ctx)
{
    uint32_t ret = 0;
    
    onetls_key_share_del(ctx->handshake->dh_para.key_share);
    ctx->handshake->dh_para.key_share = NULL;
    onetls_buffer_del_safe(&(ctx->handshake->cookie));
    memset(&(ctx->handshake->msg_tag), 0, sizeof(ctx->handshake->msg_tag));
    memset(&(ctx->handshake->secret), 0, sizeof(ctx->handshake->secret)); 

    onetls_release_cache_list(ctx->handshake);

    onetls_buffer_del_safe(&(ctx->sess_id));
    onetls_buffer_del_safe(&(ctx->server_name));
    onetls_buffer_del_safe(&(ctx->handshake->dh_para.dh_key));
    onetls_buffer_del_safe(&(ctx->handshake->handshake_packet.handshake_data));
    onetls_buffer_del_safe(&(ctx->handshake->send_cache));
    onetls_buffer_reset(ctx->message.msg_in);
    onetls_buffer_reset(ctx->message.msg_out);
    onetls_get_random(ctx);

    ctx->handshake->hs_msg_timer = 0;
    ctx->security.dtls.record_num_wr.epoch = ONETLS_EPOCH_FOR_CH_SH_HRR;
    ctx->security.dtls.record_num_rd.epoch = ONETLS_EPOCH_FOR_CH_SH_HRR;

    // 开始准备数据
    ret = onetls_gen_early_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_early_secret");
        return ret;
    }
    
    ret = onetls_gen_key_share(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_key_share");
        return ret;
    }
    ctx->sub_state = ONETLS_SUB_STATE_PREPARING;
    ctx->state = ONETLS_STATE_SEND_CNT_HELLO;
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    if (onetls_is_dtls(ctx)) {
        ctx->sub_state = ONETLS_SUB_STATE_PREPARING;
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_send_client_hello(onetls_ctx *ctx)
{
    uint32_t ret = onetls_construct_client_hello_packet(ctx);
    if (ret != ONETLS_SUCCESS) {
        return ret;
    }

    ret = onetls_hash(ctx->cipher->hkdf_hash, 
                      onetls_buffer_get_data(ctx->handshake->handshake_packet.handshake_data),
                      onetls_buffer_get_tail(ctx->handshake->handshake_packet.handshake_data),
                      ctx->handshake->handshake_packet.ch, NULL);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }

    onetls_buffer_set_cursor(ctx->message.msg_out, 0); // 放置到头部
    onetls_buffer_reset(ctx->message.msg_in); 
    ctx->handshake->hs_msg_timer = onetls_get_time();   
    ctx->sub_state = ONETLS_SUB_STATE_SENDING;
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_SEND;
    ctx->state = ONETLS_STATE_GEN_C_EARLY_TRAFFIC_SECRET;
    return ret;
}

uint32_t onetls_gen_early_traffic_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_early_traffic_secret_ex(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_hs_traffic_secret");
        return ret;
    }

    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    ctx->state = ONETLS_STATE_SEND_EARLY_DATA;
    return ret;
}

uint32_t onetls_send_early_data_process(onetls_ctx *ctx)
{
    onetls_buffer_reset(ctx->message.msg_out);	
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_READ;    // 准备读取hello

    // 有early data
    if (ctx->early_data.early_buffer != NULL) {
        uint32_t ret = onetls_send_encode_buffer(ctx, ctx->message.msg_out, ctx->early_data.early_buffer, ONETLS_MT_APPLICATION_DATA);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_send_encode_buffer early data");
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
            return ret;
        }

        onetls_buffer_del_safe(&(ctx->early_data.early_buffer));
        ctx->handshake->want_io |= ONETLS_HANDSHAKE_IO_WANT_SEND;
    }
    ctx->state = ONETLS_STATE_RECV_SVR_HELLO;
    return ONETLS_SUCCESS;
}

uint32_t onetls_recv_server_hello(onetls_ctx *ctx)
{
    uint8_t mt = 0;
    onetls_buffer *buffer = ctx->message.msg_in;
    uint32_t ret = onetls_want_a_msg(ctx, ONETLS_MT_HANDSHAKE);
    if (ret != ONETLS_SUCCESS) {
        return ret;    
    }

    onetls_buffer_set_cursor(buffer, ONETLS_RECORD_HEADER_LEN);
    onetls_buffer_get_uint8(buffer, &mt);
    if ((mt != ONETLS_SERVER_HELLO) || (ctx->handshake->msg_tag.server_hello > 1)) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_UNEXPECTED_MESSAGE);
        return ONETLS_EXCEPTION_MSG;
    }

    ret = onetls_parse_server_hello(ctx, buffer);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_recv_server_hello");
        return ret;
    }
    (ctx->handshake->msg_tag.server_hello)++;

    // 缓存消息，后面要用来计算摘要
    onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), 
                                    onetls_buffer_get_data(buffer) + ONETLS_RECORD_HEADER_LEN, 
                                    onetls_buffer_get_tail(buffer) - ONETLS_RECORD_HEADER_LEN, 0);

    ret = onetls_hash(ctx->cipher->hkdf_hash, 
                    onetls_buffer_get_data(ctx->handshake->handshake_packet.handshake_data),
                    onetls_buffer_get_tail(ctx->handshake->handshake_packet.handshake_data),
                    ctx->handshake->handshake_packet.ch_to_sh, NULL);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }
    
    onetls_buffer_reset(ctx->message.msg_in);
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    
    ctx->state = (ctx->handshake->msg_tag.server_hello_retry) ? ONETLS_STATE_SEND_CNT_HELLO : ONETLS_STATE_GEN_HS_SECRET;
    return ONETLS_SUCCESS;
}

uint32_t onetls_tls_handshake_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_hs_traffic_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_hs_traffic_secret");
        return ret;
    }    
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    ctx->state = ONETLS_STATE_GEN_S_HS_SECRET;
    return ret;
}

uint32_t onetls_tls_server_hs_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_s_hs_traffic_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_s_hs_traffic_secret");
        return ret;
    }    

    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_READ;
    ctx->state = ONETLS_STATE_RECV_SVR_EE;
    return ret;
}

uint32_t onetls_tls_recv_server_ee(onetls_ctx *ctx)
{
    uint8_t mt = 0;
    onetls_buffer *buffer = ctx->message.msg_in, *plain_buffer = NULL;
    uint32_t ret = onetls_want_a_msg(ctx, ONETLS_MT_APPLICATION_DATA);
    if (ret != ONETLS_SUCCESS) {
        return ret; 
    }

    // 解密扩展
    plain_buffer = onetls_buffer_new(onetls_buffer_get_tail(buffer));
    ret = onetls_recv_decode_buffer(ctx, plain_buffer, buffer, &mt);
    if ((ret != ONETLS_SUCCESS) || (mt != ONETLS_MT_HANDSHAKE)) {
        onetls_check_errlog(ret, "onetls_tls_recv_server_ee buffer");
        onetls_buffer_del(plain_buffer);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_DECODE_ERROR);
        return ret;
    }

    ret = onetls_tls_parse_server_ee(ctx, plain_buffer);
    if (ret != ONETLS_SUCCESS) {   // 报文不对
        onetls_buffer_del(plain_buffer);
        onetls_check_errlog(ret, "onetls_tls_parse_server_ee");
        return ret;
    }

    // 缓存消息，后面要用来计算摘要
    onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), 
                                     onetls_buffer_get_data(plain_buffer), 
                                     onetls_buffer_get_tail(plain_buffer), 0);

    onetls_buffer_reset(buffer);
    onetls_buffer_del(plain_buffer);
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_READ;
    ctx->state = ONETLS_STATE_RECV_SVR_FINISH;
    return ONETLS_SUCCESS;
}

uint32_t onetls_tls_recv_server_finish(onetls_ctx *ctx)
{
    uint8_t hash[ONETLS_MAX_MD_LEN] = { 0 }, mt = ONETLS_MT_APPLICATION_DATA, pre_out[ONETLS_MAX_MD_LEN] = { 0 };
    onetls_buffer *buffer = ctx->message.msg_in, *plain_buffer = NULL;
    uint16_t message_seq =0;
    uint32_t frag_offset = 0, frag_len = 0;
    uint32_t ret = onetls_want_a_msg(ctx, ONETLS_MT_APPLICATION_DATA), md_size = onetls_hash_size(ctx->cipher->hkdf_hash);
    if (ret != ONETLS_SUCCESS) {
        return ret;  
    }
    plain_buffer = onetls_buffer_new(onetls_buffer_get_tail(buffer));

    // 解密finish
    ret = onetls_recv_decode_buffer(ctx, plain_buffer, buffer, &mt);
    if ((ret != ONETLS_SUCCESS) || (mt != ONETLS_MT_HANDSHAKE)) {
        onetls_buffer_del(plain_buffer);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_DECODE_ERROR);
        return ret;
    }

    onetls_buffer_get_uint8(plain_buffer, &mt);
    onetls_buffer_get_uint24(plain_buffer, &ret);
    if (onetls_is_dtls(ctx)) {
        onetls_buffer_get_uint16(plain_buffer, &message_seq);
        onetls_buffer_get_uint24(plain_buffer, &frag_offset);
        onetls_buffer_get_uint24(plain_buffer, &frag_len);
        if ((frag_offset != 0) || (ret != frag_len)) { // 暂不支持分片
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_BAD_RECORD_MAC);
            return ONETLS_INVALID_PARA;
        }
        if (message_seq != ctx->handshake->msg_tag.message_seq_rd) {
            return ONETLS_INVALID_PARA;
        }
        ctx->handshake->msg_tag.message_seq_rd ++;
    }
    if ((mt != ONETLS_FINISHED) ||(ret != md_size)) {
        onetls_buffer_del(plain_buffer);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXCEPTION_MSG;   
    }

    // 计算之前的消息摘要，将头偏移走
    ret = onetls_hash(ctx->cipher->hkdf_hash, 
                      onetls_buffer_get_data(ctx->handshake->handshake_packet.handshake_data), 
                      onetls_buffer_get_tail(ctx->handshake->handshake_packet.handshake_data), hash, &md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        onetls_buffer_del(plain_buffer);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }

    onetls_hmac(ctx->cipher->hkdf_hash, ctx->handshake->secret.s_finish_key, md_size, hash, md_size, pre_out, &md_size);    

    frag_offset = 4;
    if (onetls_is_dtls(ctx)) {
        frag_offset = 12;
    }
    if (memcmp(onetls_buffer_get_data(plain_buffer) + frag_offset, pre_out, md_size) != 0) {
        onetls_check_errlog(ONETLS_EXCEPTION_MSG, "memcmp finish mac");
        onetls_buffer_del(plain_buffer);
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXCEPTION_MSG;
    }

    // 缓存消息，后面要用来计算摘要
    onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), 
                                     onetls_buffer_get_data(plain_buffer), 
                                     onetls_buffer_get_tail(plain_buffer), 0);
    onetls_buffer_del(plain_buffer);
    
    ret = onetls_hash(ctx->cipher->hkdf_hash, 
                      onetls_buffer_get_data(ctx->handshake->handshake_packet.handshake_data),
                      onetls_buffer_get_tail(ctx->handshake->handshake_packet.handshake_data),
                      ctx->handshake->handshake_packet.ch_to_sf, NULL);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }
    
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    ctx->state = ONETLS_STATE_SEND_CNT_CCS;
    return ONETLS_SUCCESS;    
}

uint32_t onetls_tls_send_client_ccs(onetls_ctx *ctx)
{
    onetls_buffer_reset(ctx->message.msg_out);
    if (ctx->handshake->msg_tag.server_ccs) {
        onetls_buffer_put_uint8(ctx->message.msg_out, ONETLS_MT_CCS);
        onetls_buffer_put_uint16(ctx->message.msg_out, ONETLS_VERSION_TLS_12);
        onetls_buffer_put_uint16(ctx->message.msg_out, 1);
        onetls_buffer_put_uint8(ctx->message.msg_out, 1);
        onetls_buffer_set_cursor(ctx->message.msg_out, 0); // 放置到头部
        ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_SEND;
    }
    ctx->state = ONETLS_STATE_SEND_EOE;
    return ONETLS_SUCCESS;  
}

uint32_t onetls_tls_send_client_eoe(onetls_ctx *ctx)
{
    onetls_buffer_reset(ctx->message.msg_out);
    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;    
    if ((ctx->handshake->msg_tag.server_ext_early_data) && (!onetls_is_dtls(ctx))) {
        uint8_t eoe[ONETLS_TLS_END_OF_EARLY_LEN] = { ONETLS_CLIENT_END_OF_EARLY_DATA, 0 };
        uint32_t ret = onetls_send_encode_data(ctx, ctx->message.msg_out, eoe, ONETLS_TLS_END_OF_EARLY_LEN, ONETLS_MT_HANDSHAKE);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_tls_send_client_eoe eoe");
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
            return ret;
        }
        // 缓存消息，后面要用来计算摘要
        onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), eoe, ONETLS_TLS_END_OF_EARLY_LEN, 0);
        ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_SEND;
    }
    ctx->state = ONETLS_STATE_GEN_C_HS_SECRET;
    return ONETLS_SUCCESS;
}

uint32_t onetls_tls_client_hs_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_c_hs_traffic_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_c_hs_traffic_secret");
        return ret;
    }

    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    ctx->state = ONETLS_STATE_SEND_CNT_FINISH;
    return ret;
}

uint32_t onetls_tls_send_client_finish(onetls_ctx *ctx)
{
    uint8_t hash[ONETLS_MAX_MD_LEN] = { 0 }, pre_out[ONETLS_MAX_MD_LEN + 13] = { 0 };
    uint32_t ret = 0, md_size = onetls_hash_size(ctx->cipher->hkdf_hash);
    uint32_t offset = 0;
    // 计算消息的摘要
    ret = onetls_hash(ctx->cipher->hkdf_hash, 
                      onetls_buffer_get_data(ctx->handshake->handshake_packet.handshake_data), 
                      onetls_buffer_get_tail(ctx->handshake->handshake_packet.handshake_data), hash, NULL);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }
    offset = 4;
    if (onetls_is_dtls(ctx)) {
        offset = 12;
    }
    ret = onetls_hmac(ctx->cipher->hkdf_hash, ctx->handshake->secret.c_finish_key, md_size, hash, md_size, pre_out + offset, &md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hmac");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }

    offset = 0;
    pre_out[offset++] = ONETLS_FINISHED;
    pre_out[offset++] = (md_size & 0x00ff0000) >> 16;
    pre_out[offset++] = (md_size & 0x0000ff00) >> 8;
    pre_out[offset++] = (md_size & 0x000000ff);
    if (onetls_is_dtls(ctx)) {
        pre_out[offset++] = (ctx->handshake->msg_tag.message_seq_wr & 0xff00) >> 8;
        pre_out[offset++] = (ctx->handshake->msg_tag.message_seq_wr & 0x00ff);
        ctx->handshake->msg_tag.message_seq_wr ++;

        offset += 3;    // 不支持分片
        pre_out[offset++] = (md_size & 0x00ff0000) >> 16;
        pre_out[offset++] = (md_size & 0x0000ff00) >> 8;
        pre_out[offset++] = (md_size & 0x000000ff);        
    }
    // 缓存消息，后面要用来计算摘要
    onetls_buffer_put_stream_realloc(&(ctx->handshake->handshake_packet.handshake_data), pre_out, md_size + offset, 0);
    ret = onetls_send_encode_data(ctx, ctx->message.msg_out, pre_out, md_size + offset, ONETLS_MT_HANDSHAKE);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;   
    }

    ret = onetls_hash(ctx->cipher->hkdf_hash, 
                      onetls_buffer_get_data(ctx->handshake->handshake_packet.handshake_data),
                      onetls_buffer_get_tail(ctx->handshake->handshake_packet.handshake_data),
                      ctx->handshake->handshake_packet.ch_to_cf, NULL);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }

    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_SEND;
    ctx->state = ONETLS_STATE_GEN_APP_SECRET;
    return ONETLS_SUCCESS;
}

uint32_t onetls_tls_app_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_master_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_master_secret");
        return ret;
    }
    
    ret = onetls_gen_c_app_traffic_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_c_app_traffic_secret");
        return ret;
    }

    ret = onetls_gen_s_app_traffic_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_s_app_traffic_secret");
        return ret;
    }

    ret = onetls_gen_res_master_secret(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_gen_res_master_secret");
        return ret;
    }

    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    ctx->state = ONETLS_STATE_PRE_OK;
    return ret;
}

uint32_t onetls_tls_pre_ok(onetls_ctx *ctx)
{
    onetls_buffer_reset(ctx->message.msg_in);
    onetls_buffer_reset(ctx->message.msg_out);

    onetls_release_cache_list(ctx->handshake);

    ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_NOTHING;
    ctx->state = ONETLS_STATE_OK;
    return ONETLS_SUCCESS;
}

uint32_t onetls_tls_recv_server_ccs(onetls_ctx *ctx)
{
    (ctx->handshake->msg_tag.server_ccs)++;
    onetls_buffer_reset(ctx->message.msg_in);
    return ONETLS_SUCCESS;
}

uint32_t onetls_tls_send_update_key(onetls_ctx* ctx, uint8_t way)
{
    uint32_t ret = 0;
    onetls_buffer *buffer = onetls_buffer_new(ONETLS_TLS_KEY_UPDATE_LEN + ONETLS_ADDITIONAL_CIPHER_LEN);
    onetls_assert(buffer != NULL);
    
    ret = onetls_tls_pkt_update_key(ctx, way, buffer);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_pkt_update_key");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_buffer_del(buffer);
        return ret;   
    }

    // 将没有发出去的报文，刷出去
    ret = onetls_flush(ctx);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_flush");
        onetls_buffer_del(buffer);
        return ret;   
    }
    
    ret = ONETLS_SOCKET_TRYAGAIN;
    while (ret == ONETLS_SOCKET_TRYAGAIN) {
        ret = onetls_send_record(ctx, buffer);
    }
    onetls_buffer_del(buffer);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_tls_send_update_key");
        return ret;
    }
    
    // 更新自己
    ret = onetls_update_traffic_key(ctx, 1);
    onetls_check_errlog(ret, "onetls_update_key 1");
    return ret;    
}

uint32_t onetls_tls_recv_key_update(onetls_ctx* ctx, onetls_buffer *buffer)
{
    uint32_t ret = 0;
    uint8_t type = 0;
    
    if (onetls_buffer_get_tail(buffer) != ONETLS_TLS_KEY_UPDATE_LEN) {
        onetls_check_errlog(ONETLS_EXCEPTION_MSG, "onetls_tls_recv_key_update len");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_BAD_RECORD_MAC);
        return ONETLS_EXCEPTION_MSG;
    }

    onetls_buffer_get_uint24(buffer, &ret);
    onetls_buffer_get_uint8(buffer, &type);
    if ((ret != 1) || (type > ONETLS_KEY_UPDATE_REQUEST)) {
        onetls_check_errlog(ONETLS_EXCEPTION_MSG, "onetls_tls_recv_key_update type");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
        return ONETLS_EXCEPTION_MSG;
    }

    ret = onetls_update_traffic_key(ctx, 0);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_update_key 0");
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        return ret;
    }

    if (type == ONETLS_KEY_UPDATE_REQUEST) {
        // 还要更新本端的
        ret = onetls_tls_send_update_key(ctx, 0);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ONETLS_EXCEPTION_MSG, "onetls_tls_recv_key_update self");
            onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_ILLEGAL_PARAMETER);
            return ONETLS_EXCEPTION_MSG;
        }
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_deal_with_nst(onetls_ctx* ctx, onetls_buffer *buffer, onetls_resumption *res_data, uint16_t res_len, uint8_t *nonce, uint8_t nonce_len)
{
    uint32_t ret = onetls_tls_parse_server_extension(ctx, buffer, ONETLS_STATE_RECV_SVR_NST);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_deal_with_nst onetls_tls_parse_server_ext");
        return ret;
    }

    res_data->cipher_id = ctx->cipher->iana_id;
    res_data->max_early_data = ctx->resumption.max_early_data_len_tmp;

    ret = onetls_gen_res_master_key(ctx, res_data, nonce, nonce_len);
    if (ret != ONETLS_SUCCESS) {    // 派生下一次的主密钥
        onetls_send_alert(ctx, ONETLS_ALERT_LEVEL_FATAL, ONETLS_ALERT_DESCRIPTION_INTERNAL_ERROR);
        onetls_check_errlog(ret, "onetls_deal_with_nst onetls_gen_res_master_key");
        return ret;
    }

    ret = ctx->resumption.ticket_cb((uint8_t *)res_data, res_len);
    onetls_check_errlog(ret, "onetls_deal_with_nst ticket_cb");
    return ret;
}

uint32_t onetls_check_dtls_hs_msg(onetls_ctx *ctx)
{
    onetls_list_node *cur_node = NULL;
    if ((ctx->state >= ONETLS_STATE_OK) || (ctx->handshake == NULL)) {
        return ONETLS_SUCCESS;
    }
    cur_node = ctx->handshake->recv_list.next;

    onetls_buffer_reset(ctx->message.msg_in);
    onetls_buffer_reset(ctx->message.msg_out);

    while (cur_node != &(ctx->handshake->recv_list)) {
        onetls_packet_node *packet_node = (onetls_packet_node *)(cur_node->v_ptr);
        onetls_assert(packet_node != NULL);

        if (packet_node->epoch != ctx->security.dtls.record_num_rd.epoch) {
            cur_node = cur_node->next;
            continue;
        }

        // 是我们要的报文        
        onetls_buffer_put_stream(ctx->message.msg_in, onetls_buffer_get_data(packet_node->packet), onetls_buffer_get_tail(packet_node->packet));
        onetls_list_del(cur_node);
        onetls_buffer_del(packet_node->packet);
        onetls_free(packet_node);
        ctx->handshake->recv_list.v_byte --;
        break;
    }

    if (onetls_buffer_get_tail(ctx->message.msg_in)) {
        onetls_buffer_del_safe(&(ctx->handshake->send_cache));
        ctx->sub_state = ONETLS_SUB_STATE_PREPARING;   
    } else {
        if (ctx->handshake->hs_msg_retry_times > ONETLS_DTLS_HS_MSG_RETRY_TIMES) {  // 不连了
            onetls_shutdown(ctx);
            return ONETLS_FAIL;
        }
        if ((onetls_get_time() - ctx->handshake->hs_msg_timer) > ONETLS_DTLS_WAITING_TIMEOUT) {
            onetls_buffer_put_stream(ctx->message.msg_out, 
                                     onetls_buffer_get_data(ctx->handshake->send_cache), 
                                     onetls_buffer_get_tail(ctx->handshake->send_cache));

            ctx->handshake->hs_msg_retry_times ++;
            ctx->handshake->hs_msg_timer = onetls_get_time();   
            ctx->sub_state = ONETLS_SUB_STATE_SENDING;
            ctx->handshake->want_io = ONETLS_HANDSHAKE_IO_WANT_SEND;
        }
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_negoatiate(onetls_ctx *ctx)
{
    uint32_t ret = ONETLS_SUCCESS;
    onetls_handshake *handshake = ctx->handshake;

    while (ctx->state < ONETLS_STATE_OK) {
        // 如果中间那里出错了，且不可恢复，就直接shutdown
        if (ctx->shutdown > 0) {
            onetls_shutdown_inner(ctx);
            return ctx->shutdown;
        }

        if (handshake->want_io & ONETLS_HANDSHAKE_IO_WANT_SEND) {   // 有数据要写
            ret = onetls_send_record(ctx, ctx->message.msg_out);
            if (ret != ONETLS_SUCCESS) {
                return ret;
            }
            handshake->want_io &= ~ONETLS_HANDSHAKE_IO_WANT_SEND;
            if (onetls_is_dtls(ctx)) {
                ctx->sub_state = ONETLS_SUB_STATE_WAITING;
            }            
        }
        
        if (handshake->want_io & ONETLS_HANDSHAKE_IO_WANT_READ) {   // 有数据要读
            ret = onetls_recv_record(ctx, ctx->message.msg_in);
            if (ret != ONETLS_SUCCESS) {
                return ret;
            }

            // 主要用于判断dtls的报文是否准备好，重传还是排序
            if (onetls_check_record(ctx) != ONETLS_SUCCESS) {
                continue;
            }

            if (onetls_is_dtls(ctx)) {
                if (ctx->sub_state != ONETLS_SUB_STATE_PREPARING) {
                    handshake->want_io |= ONETLS_HANDSHAKE_IO_WANT_READ;
                    continue;   // 未收发到想要的报文，就不进入下一个阶段
                }
            }
        }

        // 进入状态机
        ret = g_onetls_state_machine[ctx->state](ctx);
        if (ret == ONETLS_SOCKET_TRYAGAIN) {
            continue;
        } else if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_ctx_negotiate[%d]", ctx->state);
            return ret;
        }
    }
    return ret;
}

