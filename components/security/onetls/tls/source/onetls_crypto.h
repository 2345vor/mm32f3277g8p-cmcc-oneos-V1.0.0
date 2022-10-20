#ifndef __ONETLS_CRYPTO_H__
#define __ONETLS_CRYPTO_H__
#include "onetls_client.h"
#include "onetls_buffer.h"

#define ONETLS_MAX_MD_LEN 64
#define ONETLS_MAX_IV_LEN 16

#define ONETLS_DH_PB_KEY_LEN (133)

#define ONETLS_DH_PSK_ONLY   (0)
#define ONETLS_DH_PSK_DHE    (1)

typedef struct {
    uint16_t iana_id;
} onetls_sign_alg;

typedef struct {
    uint16_t    iana_id;        // 套件ianaID
    uint16_t    key_type;       // 套件类型
    uint32_t    key_alg;        // 算法ID
    uint32_t    hkdf_hash;      // hkdf的算法
    uint8_t     key_len;        // 秘钥的长度
    uint8_t     iv_len;         // iv的长度
    uint8_t     tag_len;        // 密文尾部补充长度
} onetls_cipher;

typedef struct {
    uint16_t    iana_id;
    uint16_t    key_bits;    
    uint32_t    kex;
    uint8_t     curve;
} onetls_dh_groups;

typedef struct {
    const onetls_dh_groups *group;
    void   *handle;                 // dh 私钥的键值； 用openssl时，可以是私钥，用psa时，可以是索引
    uint8_t public_len;
    uint8_t public_key[ONETLS_DH_PB_KEY_LEN];
} onetls_key_share;

// 初始化加解密库，主要是看第三方库的使用规则
uint32_t onetls_crypto_init(void);

// 获取配置的算法套件
const onetls_cipher *onetls_get_ciphers(uint32_t *num);
const onetls_cipher *onetls_get_cipher_byid(uint16_t id);
const onetls_sign_alg *onetls_get_sign_algs(uint32_t *num);
const onetls_dh_groups *onetls_get_support_groups(uint32_t *num);

// 计算dh参数
uint32_t onetls_gen_key_share(onetls_ctx *ctx);
void onetls_key_share_del(onetls_key_share *key_share);
uint32_t onetls_gen_kex_with_pub_key(onetls_ctx *ctx, onetls_key_share *key_share, uint8_t* pub_key, uint32_t pub_len);

// 获取默认的hmac算法，TLS1.3的带外默认都是sha256
uint32_t onetls_get_default_hkdf_hash();

void onetls_get_rnd(uint8_t *data, uint32_t len);

// 创建加解密上下文
void *onetls_crypto_create_ctx(void);
void  onetls_crypto_delete_ctx(void *crypto_ctx);
uint32_t onetls_crypto_ctx_init(void *ctx, const onetls_cipher *cipher, uint8_t *key, uint8_t usage);

uint32_t onetls_aead_decrypt(onetls_ctx *ctx, 
                             uint8_t *nonce, 
                             uint8_t *add, uint32_t add_len, 
                             uint8_t *in, uint32_t in_len, 
                             uint8_t *mt, 
                             uint8_t *out, uint32_t *out_len);
uint32_t onetls_aead_encrypt(onetls_ctx *ctx, 
                             uint8_t *nonce, 
                             uint8_t *add, uint32_t add_len, 
                             uint8_t *in, uint32_t in_len, 
                             uint8_t *mt, 
                             uint8_t *out, uint32_t *out_len);

uint32_t onetls_aes_ecb_encrypt(uint8_t *key, uint32_t key_len, uint8_t *input, uint8_t *output);   // 16
uint32_t onetls_aes_ecb_decrypt(uint8_t *key, uint32_t key_len, uint8_t *input, uint8_t *output);

// 获取hash算法长度
uint32_t onetls_hash_size(uint32_t hash_id);

// 计算摘要
uint32_t onetls_hash(uint32_t hash_id, 
                     const uint8_t *in, uint32_t in_len, 
                     uint8_t *out, uint32_t *out_len);

// hmac算法集合
uint32_t onetls_hmac(uint32_t hash_id,
                     const uint8_t *key, uint32_t key_len,
                     const uint8_t *data, uint32_t data_len,
                     uint8_t *out, uint32_t *out_len);

#endif
