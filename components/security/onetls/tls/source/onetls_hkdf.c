#include "onetls_hkdf.h"
#include "onetls_lib.h"

static const uint8_t g_onetls_hkdf_zero[ONETLS_MAX_MD_LEN] = { 0 };
static uint32_t onetls_hkdf_extract(uint32_t hash_id, 
                                    const uint8_t *salt, uint32_t salt_len, 
                                    const uint8_t *key, uint32_t key_len, 
                                    uint8_t *out, uint32_t out_len)
{
    if (salt == NULL) {
        salt = g_onetls_hkdf_zero;
        salt_len = onetls_hash_size(hash_id);
    }
    if (key == NULL) {
        key = g_onetls_hkdf_zero;
        key_len = onetls_hash_size(hash_id);
    }
    return onetls_hmac(hash_id, salt, salt_len, key, key_len, out, &out_len);
}

uint32_t onetls_hkdf_expand(onetls_ctx *ctx,
                            const uint8_t *info, uint32_t info_len,
                            const uint8_t *key, uint32_t key_len,
                            uint8_t *out, uint32_t out_len)
{
    uint32_t md_size = onetls_hash_size(ctx->cipher->hkdf_hash), done_len = 0, ret = 0;
    uint8_t pre_out[ONETLS_MAX_MD_LEN * 2] = { 0 }, pre_out_2[ONETLS_MAX_MD_LEN] = { 0 };
    uint8_t rtt = out_len / md_size, offset;
    
    // 对齐
    if ((out_len % md_size) != 0) rtt ++;    
    for (offset = 1; offset <= rtt; offset++) {
        uint8_t *p = pre_out;
        if (offset != 1) {
            memcpy(p, out, done_len);
            p += done_len;
        }
        memcpy(p, info, info_len);
        p += info_len;
        *p++ = offset;
        ret = onetls_hmac(ctx->cipher->hkdf_hash, key, key_len, pre_out, p - pre_out, pre_out_2, &md_size);
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_hmac");
            return ret;
        }
        memcpy(out, pre_out_2, onetls_min(out_len, md_size));
        done_len += onetls_min(out_len, md_size);
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_hkdf_expand_label(onetls_ctx *ctx, 
                                  const uint8_t *key, uint32_t key_len, 
                                  const uint8_t *label, uint32_t label_len, 
                                  const uint8_t *info, uint32_t info_len, 
                                  uint8_t *out, uint32_t out_len)
{
    uint8_t hkdf_label[ONETLS_HKDF_MAX_INFO_LEN + ONETLS_MAX_MD_LEN + 4] = { 0 }, *p = hkdf_label;
    const char *protocol = onetls_is_dtls(ctx) ? "dtls13" : "tls13 ";
    uint32_t protocol_len = strlen(protocol);

    if (info_len > ONETLS_HKDF_MAX_INFO_LEN) {
        onetls_check_errlog(ONETLS_INVALID_PARA, "onetls_hkdf_expand_label %s-%d %d", label, label_len, info_len);
        return ONETLS_INVALID_PARA;
    }

    *p++ = (out_len & 0x0000ff00) >> 8;
    *p++ = (out_len & 0x000000ff);
    *p++ = protocol_len + label_len;
    memcpy(p, protocol, protocol_len);
    p += protocol_len;
    memcpy(p, label, label_len);
    p += label_len;

    *p++ = info_len;
    memcpy(p, info, info_len);
    p += info_len;

    return onetls_hkdf_expand(ctx, hkdf_label, p - hkdf_label, key, key_len, out, out_len);
}

uint32_t onetls_hkdf_derive_finished(onetls_ctx *ctx, const uint8_t *key, uint32_t key_len, uint8_t *out, uint32_t out_len)
{
    static const uint8_t finished_label[] = "finished";
    return onetls_hkdf_expand_label(ctx, key, key_len, finished_label, sizeof(finished_label) - 1, NULL, 0, out, out_len);
}

uint32_t onetls_hkdf_derive_key(onetls_ctx *ctx, const uint8_t *key, uint32_t key_len, uint8_t *out, uint32_t out_len)
{
    static const uint8_t key_label[] = "key";
    return onetls_hkdf_expand_label(ctx, key, key_len, key_label, sizeof(key_label) - 1, NULL, 0, out, out_len);    
}

uint32_t onetls_hkdf_derive_iv(onetls_ctx *ctx, const uint8_t *key, uint32_t key_len, uint8_t *out, uint32_t out_len)
{
    static const uint8_t iv_label[] = "iv";
    return onetls_hkdf_expand_label(ctx, key, key_len, iv_label, sizeof(iv_label) - 1, NULL, 0, out, out_len);    
}

uint32_t onetls_hkdf_derive_sn_key(onetls_ctx *ctx, 
                                   const uint8_t *key, uint32_t key_len, 
                                   uint8_t *out, uint32_t out_len)
{
    static const uint8_t sn_label[] = "sn";
    return onetls_hkdf_expand_label(ctx, key, key_len, sn_label, sizeof(sn_label) - 1, NULL, 0, out, out_len);   
}

uint32_t onetls_hkdf_derive_secret(onetls_ctx *ctx, 
                                   const uint8_t *label, uint32_t label_len,
                                   const uint8_t *secret, uint32_t secret_len,
                                   const uint8_t *in, uint32_t in_len,
                                   uint8_t *out, uint32_t out_len)
{
    uint8_t pre_out[ONETLS_MAX_MD_LEN] = { 0 };
    uint32_t pre_out_len = ONETLS_MAX_MD_LEN;

    uint32_t ret = onetls_hash(ctx->cipher->hkdf_hash, in, in_len, pre_out, &pre_out_len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hash");
        return ret;
    }    

    ret = onetls_hkdf_expand_label(ctx, secret, secret_len, label, label_len, pre_out, pre_out_len, out, out_len);
    onetls_check_errlog(ret, "onetls_hkdf_expand_label");
    return ret;
}

uint32_t onetls_gen_early_secret_inner(onetls_ctx *ctx, uint8_t *secret, uint8_t *hint, uint32_t hint_len)
{
    uint32_t ret = 0;

    if ((ctx->psk_info.hint == NULL) || 
        (ctx->psk_info.out_band_psk == NULL)) {
        return ONETLS_NO_PSK_HINT;
    }
    memset(secret, 0, ONETLS_MAX_MD_LEN);

    // 计算，没有盐
    ret = onetls_hkdf_extract(onetls_get_default_hkdf_hash(), 
                              NULL, 0, 
                              onetls_buffer_get_data(ctx->psk_info.out_band_psk), onetls_buffer_get_tail(ctx->psk_info.out_band_psk), 
                              secret, onetls_hash_size(onetls_get_default_hkdf_hash()));
    onetls_check_errlog(ret, "onetls_hkdf_extract");
    return ret;
}

uint32_t onetls_gen_early_secret(onetls_ctx *ctx)
{
    uint32_t ret = onetls_gen_early_secret_inner(ctx, 
                                                 ctx->handshake->secret.early_secret[0], 
                                                 onetls_buffer_get_data(ctx->psk_info.hint), 
                                                 onetls_buffer_get_tail(ctx->psk_info.hint));
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_gen_early_secret_inner 0");
        return ret;
    }

    if (ctx->psk_info.ticket != NULL) {
        ret = onetls_hkdf_extract(ctx->cipher->hkdf_hash, 
                                  NULL, 0, 
                                  ctx->security.master_key, onetls_hash_size(ctx->cipher->hkdf_hash), 
                                  ctx->handshake->secret.early_secret[1], onetls_hash_size(ctx->cipher->hkdf_hash));
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_gen_early_secret_inner 1");
            return ret;
        }
    }
    return ret;
}

uint32_t onetls_gen_secret(onetls_ctx *ctx, uint8_t *salt, uint32_t salt_len, uint8_t *in_secret, uint8_t *out_secret)
{
    static const uint8_t derived_label[] = "derived";  
    uint8_t pre_secret[ONETLS_MAX_MD_LEN] = { 0 };    
    uint32_t md_size = onetls_hash_size(ctx->cipher->hkdf_hash), ret;
    
    memset(out_secret, 0, ONETLS_MAX_MD_LEN);
        
    // 生成预主密钥
    ret = onetls_hkdf_derive_secret(ctx, derived_label, sizeof(derived_label) - 1, in_secret, md_size, NULL, 0, pre_secret, md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_derive_secret 0");
        return ret;
    }   

    ret = onetls_hkdf_extract(ctx->cipher->hkdf_hash, pre_secret, md_size, salt, salt_len, out_secret, md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_extract 0");
        return ret;
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_gen_traffic_secret_key_iv(onetls_ctx *ctx, 
                                          const char *label, 
                                          uint8_t *salt, uint32_t salt_len, 
                                          uint8_t *in_secret, 
                                          uint8_t *out_secret, 
                                          uint8_t *key, uint32_t key_len,
                                          uint8_t *iv, uint32_t iv_len,
                                          uint8_t *finish,
                                          uint8_t *sn_key)
{
    uint32_t md_size = onetls_hash_size(ctx->cipher->hkdf_hash), ret = 0;
    
    memset(out_secret, 0, md_size);
    memset(key, 0, key_len);
    memset(iv, 0, iv_len);
    memset(sn_key, 0, key_len);

    ret = onetls_hkdf_expand_label(ctx, in_secret, md_size, (const uint8_t *)label, strlen(label), salt, salt_len, out_secret, md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_expand_label");
        return ret;
    }

    ret = (key == NULL) ? ONETLS_SUCCESS : onetls_hkdf_derive_key(ctx, out_secret, md_size, key, key_len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_derive_key");
        return ret;
    }

    ret = (iv == NULL) ? ONETLS_SUCCESS : onetls_hkdf_derive_iv(ctx, out_secret, md_size, iv, iv_len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_derive_iv");
        return ret;
    }

    ret = (finish == NULL) ? ONETLS_SUCCESS : onetls_hkdf_derive_finished(ctx, out_secret, md_size, finish, md_size);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_derive_finished");
        return ret;
    }
    ret = (sn_key == NULL) ? ONETLS_SUCCESS : onetls_hkdf_derive_sn_key(ctx, out_secret, md_size, sn_key, key_len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "onetls_hkdf_derive_finished");
        return ret;
    }
    return ONETLS_SUCCESS;
}
