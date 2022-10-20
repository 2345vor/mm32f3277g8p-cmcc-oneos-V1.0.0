#include "onetls_crypto.h"
#include "onetls_lib.h"

#ifdef OS_HWCRYPTO_USING_RNG
    #include "hwcrypto/hw_rng.h"
#endif

// 目前一整套都是基于PSA来实现的
#ifdef ONETLS_USING_PSA
    #include "psa/crypto.h"
    #include "mbedtls/entropy.h"
    #include "mbedtls/aes.h"
#endif

const static onetls_cipher g_onetls_ciphers[] = {
#ifdef ONETLS_USING_PSA
    #if defined ONETLS_ENABLE_TLS_AES_128_GCM_SHA256
        {0x1301, PSA_KEY_TYPE_AES,      PSA_ALG_GCM,                PSA_ALG_SHA_256, 16, 12, 16},   // TLS_AES_128_GCM_SHA256
    #endif
    #if defined ONETLS_ENABLE_TLS_CHACHA20_POLY1305_SHA256
        {0x1303, PSA_KEY_TYPE_CHACHA20, PSA_ALG_CHACHA20_POLY1305,  PSA_ALG_SHA_256, 32, 12, 16},   // TLS_CHACHA20_POLY1305_SHA256    
    #endif
    #if defined ONETLS_ENABLE_TLS_AES_128_CCM_SHA256
        {0x1304, PSA_KEY_TYPE_AES,      PSA_ALG_CCM,                PSA_ALG_SHA_256, 16, 12, 16},   // TLS_AES_128_CCM_SHA256
    #endif
    #if defined ONETLS_ENABLE_TLS_AES_128_CCM_8_SHA256
        {0x1305, PSA_KEY_TYPE_AES,      PSA_ALG_CCM,                PSA_ALG_SHA_256, 16, 12, 8},    // TLS_AES_128_CCM_8_SHA256
    #endif
    #if defined ONETLS_ENABLE_TLS_AES_256_GCM_SHA384
        {0x1302, PSA_KEY_TYPE_AES,      PSA_ALG_GCM,                PSA_ALG_SHA_384, 32, 12, 16},   // TLS_AES_256_GCM_SHA384  
    #endif
#endif
};

// 配置支持的group
static const onetls_dh_groups g_onetls_support_groups[] = {
#ifdef ONETLS_USING_PSA
    #if defined ONETLS_ENABLE_ECDHE_X25519
        {0x001D, 256, PSA_ALG_ECDH, PSA_ECC_CURVE_CURVE25519},   // x25519
    #endif
    #if defined ONETLS_ENABLE_ECDHE_X448
        {0x001E, 448, PSA_ALG_ECDH, PSA_ECC_CURVE_CURVE448},     // x448
    #endif
    #if defined ONETLS_ENABLE_ECDHE_SECP256R1
        {0x0017, 256, PSA_ALG_ECDH, PSA_ECC_CURVE_SECP256R1},    // secp256r1
    #endif
    #if defined ONETLS_ENABLE_ECDHE_SECP384R1
        {0x0018, 384, PSA_ALG_ECDH, PSA_ECC_CURVE_SECP384R1},    // secp384r1
    #endif
    #if defined ONETLS_ENABLE_ECDHE_SECP521R1
        {0x0019, 521, PSA_ALG_ECDH, PSA_ECC_CURVE_SECP521R1},    // secp521r1
    #endif
#endif
};

// 配置签名算法
static const onetls_sign_alg g_onetls_sign_algs[] = {
    { 0x0807 },     // ed25519
    { 0x0804 },     // rsa_pss_rsae_sha256
    { 0x0403 },     // ecdsa_secp256r1-sha256
    { 0x0401 },     // rsa_pkcs1_sha256
};

uint32_t onetls_crypto_init()
{
    uint32_t ret = 0;
    // 使用硬件真随机数喂种子
#ifdef OS_HWCRYPTO_USING_RNG
    srand(os_hwcrypto_rng_update());
#endif
    ret = psa_crypto_init();
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "psa_crypto_init");
        return ret;
    }
    return ONETLS_SUCCESS;
}

static uint32_t onetls_makeup_psa_key_handle(psa_key_handle_t *handle, 
                                             const uint8_t *key, uint32_t key_len, 
                                             uint16_t key_type, uint32_t key_usage, uint32_t key_alg)
{
#ifdef ONETLS_USING_PSA    
    psa_status_t status = PSA_SUCCESS;
    psa_key_attributes_t attributes = psa_key_attributes_init();
    /* 设置Key的属性 */
    psa_set_key_type(&attributes, key_type);
    psa_set_key_usage_flags(&attributes, key_usage);
    psa_set_key_algorithm(&attributes, key_alg);

    /* 导入密钥 */
    status = psa_import_key(&attributes, key, key_len, handle);
    psa_reset_key_attributes(&attributes);
    if (status != ONETLS_SUCCESS) {
        onetls_check_errlog(status, "psa_import_key");
        return status;
    }
    return ONETLS_SUCCESS;
#else
    #error ("onetls_makeup_psa_key_handle")
#endif      
}

static uint32_t onetls_gen_dh_para(const onetls_dh_groups *dh, onetls_key_share *share_key)
{
#ifdef ONETLS_USING_PSA     
    uint32_t ret = 0;
    size_t out_len = 0;
    psa_key_attributes_t attributes = psa_key_attributes_init();

    share_key->handle = onetls_malloc(sizeof(psa_key_handle_t));
    onetls_assert(share_key->handle);
    memset(share_key->handle, 0, sizeof(psa_key_handle_t));

    if (dh->kex == PSA_ALG_ECDH) {
        psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(dh->curve));
    } else if (dh->kex == PSA_ALG_FFDH) {
        psa_set_key_type(&attributes, PSA_KEY_TYPE_DH_KEY_PAIR(dh->curve));
    }

    psa_set_key_bits(&attributes, dh->key_bits);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_EXPORT | PSA_KEY_USAGE_DERIVE);
    psa_set_key_algorithm(&attributes, dh->kex);   

    /* 产生key */
    ret = psa_generate_key(&attributes, share_key->handle);
    psa_reset_key_attributes(&attributes);
    if (ret != ONETLS_SUCCESS) {
        onetls_free(share_key->handle);
        share_key->handle = NULL;
        onetls_check_errlog(ret, "psa_generate_key");    
        return ret;
    }

    ret = psa_export_public_key(*(psa_key_handle_t*)(share_key->handle), share_key->public_key, sizeof(share_key->public_key), &(out_len));
    if (ret != ONETLS_SUCCESS) {
        onetls_free(share_key->handle);
        share_key->handle = NULL;
        onetls_check_errlog(ret, "psa_export_public_key");    
        return ret;
    }
    share_key->public_len = (uint32_t)out_len;
    return ret;
#else
    #error ("onetls_gen_dh_para")
#endif     
}

static onetls_key_share *onetls_key_share_new()
{
    onetls_key_share *key_share = NULL;
    uint32_t num = 0, offset = 0, ret = 0;
    const onetls_dh_groups *groups = onetls_get_support_groups(&num);
    if (num == 0) {
        return NULL;
    }

    key_share = (onetls_key_share *)onetls_malloc(sizeof(onetls_key_share) * num);
    onetls_assert(key_share != NULL);
    memset(key_share, 0, sizeof(onetls_key_share) * num);

    for (offset = 0; offset < num; offset ++) { // 创建key_share
        key_share[offset].group = &(groups[offset]);
        ret = onetls_gen_dh_para(key_share[offset].group, &(key_share[offset]));
        if (ret != ONETLS_SUCCESS) {
            onetls_check_errlog(ret, "onetls_gen_dh_para");
            onetls_free(key_share);
            return NULL;
        }
    }
    return key_share;
}

static uint32_t onetls_gen_kex_agreement(uint32_t kex_type, 
                                         void *private_key, 
                                         uint8_t* pub_key, uint32_t pub_len, 
                                         uint8_t *out, uint32_t out_size, uint32_t *out_len)
{
#ifdef ONETLS_USING_PSA
    uint32_t ret = psa_raw_key_agreement(kex_type, 
                                         *(psa_key_handle_t*)(private_key), 
                                         pub_key, pub_len, 
                                         out, out_size, out_len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "psa_raw_key_agreement");
        return ret;
    }
    return ret;
#else
    #error ("onetls_gen_kex_agreement")
#endif  
}

const onetls_dh_groups *onetls_get_support_groups(uint32_t *num)
{
    if (num != NULL) {
        *num = sizeof(g_onetls_support_groups) / sizeof(onetls_dh_groups);
    }
    return g_onetls_support_groups;
}

const onetls_cipher *onetls_get_ciphers(uint32_t *num)
{
    if (num != NULL) {
        *num = sizeof(g_onetls_ciphers) / sizeof(onetls_cipher);
    }
    return g_onetls_ciphers;
}

const onetls_cipher *onetls_get_cipher_byid(uint16_t id)
{
    uint8_t offset = 0;
    for (offset = 0; offset < sizeof(g_onetls_ciphers) / sizeof(onetls_cipher); offset++) {
        if (g_onetls_ciphers[offset].iana_id == id) {
            return &g_onetls_ciphers[offset];
        }
    }
    return NULL;
}

const onetls_sign_alg *onetls_get_sign_algs(uint32_t *num)
{
    if (num != NULL) {
        *num = sizeof(g_onetls_sign_algs) / sizeof(onetls_sign_alg);
    }
    return g_onetls_sign_algs;
}

uint32_t onetls_get_default_hkdf_hash()
{
#ifdef ONETLS_USING_PSA     
    return PSA_ALG_SHA_256;
#else    
    #error ("onetls_get_default_hkdf_hash")
#endif         
}

void onetls_get_rnd(uint8_t *data, uint32_t len)
{
#ifdef ONETLS_USING_PSA 

    psa_generate_random(data, len);

#elif defined OS_HWCRYPTO_USING_RNG

    uint32_t offset = 0;
    while (offset < len) {
        data[offset++] = rand() & 0xff;
    }

#else    
    #error ("onetls_get_rnd")
#endif 
}

void *onetls_crypto_create_ctx(void)
{
#ifdef ONETLS_USING_PSA    
    psa_key_handle_t *handle = (psa_key_handle_t *)onetls_malloc(sizeof(psa_key_handle_t));
    onetls_assert(handle != NULL);
    memset(handle, 0, sizeof(psa_key_handle_t));
    return handle;
#else
    #error ("onetls_crypto_create_ctx")
#endif
}

void onetls_crypto_delete_ctx(void *crypto_ctx)
{
#ifdef ONETLS_USING_PSA     
    psa_destroy_key(*(psa_key_handle_t *)crypto_ctx);
    onetls_free(crypto_ctx);
#else
    #error ("onetls_crypto_delete_ctx")
#endif    
}

uint32_t onetls_crypto_ctx_init(void *ctx, const onetls_cipher *cipher, uint8_t *key, uint8_t usage)
{
#ifdef ONETLS_USING_PSA
    uint32_t status = 0;    
    // 初始化秘钥
    psa_destroy_key(*(psa_key_handle_t *)ctx);
    
    // 重新注入密钥
    status = onetls_makeup_psa_key_handle((psa_key_handle_t *)ctx, 
                                          key, 
                                          cipher->key_len, 
                                          cipher->key_type, 
                                          usage ? PSA_KEY_USAGE_ENCRYPT : PSA_KEY_USAGE_DECRYPT, 
                                          PSA_ALG_AEAD_WITH_TAG_LENGTH(cipher->key_alg, cipher->tag_len));
    if (status != ONETLS_SUCCESS) {
        onetls_check_errlog(status, "psa_import_key");
        psa_destroy_key(*(psa_key_handle_t *)ctx);
        return status;
    }
    return ONETLS_SUCCESS;
#else
    #error ("onetls_crypto_ctx_init")
#endif       
}

uint32_t onetls_aead_decrypt(onetls_ctx *ctx, 
                             uint8_t *nonce, 
                             uint8_t *add, uint32_t add_len, 
                             uint8_t *in, uint32_t in_len, 
                             uint8_t *mt, 
                             uint8_t *out, uint32_t *out_len)
{
#ifdef ONETLS_USING_PSA
    size_t out_len_t = *out_len + 1;

    // TODO: 这里必须要优化了，看后面有没有update的接口出来
    uint8_t *tmp_for_test = (uint8_t *)onetls_malloc(out_len_t);
    onetls_assert(tmp_for_test != NULL);

    uint32_t status = psa_aead_decrypt(*(mbedtls_svc_key_id_t*)(ctx->de_ctx),
                                       PSA_ALG_AEAD_WITH_TAG_LENGTH(ctx->cipher->key_alg, ctx->cipher->tag_len),  
                                       nonce, ctx->cipher->iv_len,
                                       add, add_len,
                                       in, in_len, tmp_for_test, out_len_t, &out_len_t);
    if (status != ONETLS_SUCCESS) {
        onetls_check_errlog(status, "psa_aead_decrypt");
        onetls_free(tmp_for_test);
        return status;
    }

    *out_len = out_len_t - 1;
    *mt = tmp_for_test[*out_len];
    memcpy(out, tmp_for_test, *out_len);
    onetls_free(tmp_for_test);
    return ONETLS_SUCCESS;
#else
    #error ("onetls_aead_decrypt")
#endif      
}

uint32_t onetls_aead_encrypt(onetls_ctx *ctx, 
                             uint8_t *nonce, 
                             uint8_t *add, uint32_t add_len, 
                             uint8_t *in, uint32_t in_len, 
                             uint8_t *mt, 
                             uint8_t *out, uint32_t *out_len)
{
#ifdef ONETLS_USING_PSA     
    size_t out_len_t = *out_len;
    uint32_t status;

    // TODO: 这里必须要优化了，看后面有没有update的接口出来
    // TODO: 这里这种只能是临时写法。正式版本，不能这么些，太挫了。
    uint8_t *tmp_for_test = (uint8_t *)onetls_malloc(in_len + 1);
    memcpy(tmp_for_test, in, in_len);
    tmp_for_test[in_len] = *mt;

    status = psa_aead_encrypt(*(mbedtls_svc_key_id_t*)(ctx->en_ctx),
                              PSA_ALG_AEAD_WITH_TAG_LENGTH(ctx->cipher->key_alg, ctx->cipher->tag_len),  
                              nonce, ctx->cipher->iv_len, 
                              add, add_len, 
                              tmp_for_test, in_len + 1, 
                              out, out_len_t, &out_len_t);
    if (status != ONETLS_SUCCESS) {
        onetls_check_errlog(status, "psa_aead_encrypt");
        onetls_free(tmp_for_test);
        return status;
    }
    *out_len = out_len_t;
    onetls_free(tmp_for_test);
    return status;
#else
    #error ("onetls_aead_encrypt")
#endif       
}

void onetls_key_share_del(onetls_key_share *key_share)
{
    uint32_t offset = 0, num = 0;
    if (key_share == NULL) {
        return;
    }
    onetls_get_support_groups(&num);
    for (offset = 0; offset < num; offset ++) {
        psa_destroy_key(*(psa_key_handle_t*)(key_share[offset].handle));
        onetls_free(key_share[offset].handle);
    }
    onetls_free(key_share);
}

uint32_t onetls_gen_key_share(onetls_ctx *ctx)
{
    uint32_t num = 0;
    onetls_get_support_groups(&num);

    ctx->handshake->dh_para.key_share = onetls_key_share_new();
    if ((num > 0) && (ctx->handshake->dh_para.key_share == NULL)) {
        onetls_check_errlog(ONETLS_FAIL, "onetls_key_share_new");
        return ONETLS_FAIL;
    }
    return ONETLS_SUCCESS;
}

uint32_t onetls_gen_kex_with_pub_key(onetls_ctx *ctx, onetls_key_share *key_share, uint8_t* pub_key, uint32_t pub_len)
{
    uint8_t raw_shared_secret[ONETLS_DH_PB_KEY_LEN] = { 0 };
    size_t out_len = 0;
    uint32_t ret = 0;

    ret = onetls_gen_kex_agreement(key_share->group->kex, 
                                   key_share->handle,
                                   pub_key, pub_len, 
                                   raw_shared_secret, sizeof(raw_shared_secret), &out_len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "psa_raw_key_agreement");
        return ret;
    }
    
    onetls_buffer_del(ctx->handshake->dh_para.dh_key);
    ctx->handshake->dh_para.dh_key = onetls_buffer_new(out_len);
    onetls_buffer_put_stream(ctx->handshake->dh_para.dh_key, raw_shared_secret, out_len);
    
    memset(raw_shared_secret, 0, sizeof(raw_shared_secret));
    return ONETLS_SUCCESS;
}

uint32_t onetls_hash_size(uint32_t hash_id)
{
#ifdef ONETLS_USING_PSA
    return PSA_HASH_SIZE(hash_id);
#else
    #error ("onetls_hash_size")
#endif        
}

uint32_t onetls_hash(uint32_t hash_id, const uint8_t *in, uint32_t in_len, uint8_t *out, uint32_t *out_len)
{
#ifdef ONETLS_USING_PSA    
    psa_status_t ret = 0;
    size_t len = ONETLS_MAX_MD_LEN;
    
    // 允许传入和不传入
    if (out_len != NULL) {
        len = *out_len;
        *out_len = 0;
    }

    // 计算摘要
    ret = psa_hash_compute(hash_id, in, in_len, out, len, &len);
    if (ret != ONETLS_SUCCESS) {
        onetls_check_errlog(ret, "psa_hash_compute");
        return ret;
    }

    // 反馈长度
    if (out_len != NULL) {
        *out_len = (uint32_t)len;
    }
    return ret;
#else
    #error ("onetls_hash")
#endif      
}

uint32_t onetls_hmac(uint32_t hash_id,
                     const uint8_t *key, uint32_t key_len,
                     const uint8_t *data, uint32_t data_len,
                     uint8_t *out, uint32_t *out_len)
{
#ifdef ONETLS_USING_PSA     
    psa_status_t status = PSA_SUCCESS;
    psa_key_handle_t      handle;
    psa_mac_operation_t operation = psa_mac_operation_init();
    size_t len = *out_len;

    /* 导入密钥 */
    status = onetls_makeup_psa_key_handle(&handle, key, key_len, PSA_KEY_TYPE_HMAC, PSA_KEY_USAGE_SIGN, PSA_ALG_HMAC(hash_id));
    if (status != ONETLS_SUCCESS) {
        onetls_check_errlog(status, "psa_import_key");
        psa_destroy_key(handle);
        return status;
    }

    /* 创建mac对象 */
    psa_mac_sign_setup(&operation, handle, PSA_ALG_HMAC(hash_id));
    
    /* 更新消息 */
    status = psa_mac_update(&operation, data, data_len);
    onetls_check_errlog(status, "psa_mac_update");

    /* 完成消息MAC计算 */
    status = psa_mac_sign_finish(&operation, out, len, &len);
    onetls_check_errlog(status, "psa_mac_sign_finish");     

    *out_len = (uint32_t)len;
    psa_destroy_key(handle);
    psa_mac_abort(&operation);
    return status;
#else
    #error ("onetls_hmac")
#endif      
}

uint32_t onetls_aes_ecb_encrypt(uint8_t *key, uint32_t key_len, uint8_t *input, uint8_t *output)
{
#ifdef ONETLS_USING_PSA 
    int ret = 0;
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);    
    mbedtls_aes_setkey_enc(&aes_ctx, key, key_len * 8);
    ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, input, output);
    mbedtls_aes_free(&aes_ctx);
    return ret;
#else
    #error ("onetls_aes_ecb_encrypt")
#endif   
}

uint32_t onetls_aes_ecb_decrypt(uint8_t *key, uint32_t key_len, uint8_t *input, uint8_t *output)
{
#ifdef ONETLS_USING_PSA 
    int ret = 0;
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);    
    mbedtls_aes_setkey_dec(&aes_ctx, key, key_len * 8);
    ret = mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, input, output);
    mbedtls_aes_free(&aes_ctx);
    return ret;
#else
    #error ("onetls_aes_ecb_decrypt")
#endif 
}