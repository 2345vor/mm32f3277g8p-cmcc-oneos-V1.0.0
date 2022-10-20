#ifndef __ONETLS_HKDF_H__
#define __ONETLS_HKDF_H__
#include "onetls_client.h"

#define ONETLS_HKDF_MAX_INFO_LEN 255

uint32_t onetls_hkdf_expand(onetls_ctx *ctx,
                            const uint8_t *info, uint32_t info_len,
                            const uint8_t *key, uint32_t key_len,
                            uint8_t *out, uint32_t out_len);

uint32_t onetls_hkdf_expand_label(onetls_ctx *ctx, 
                                  const uint8_t *key, uint32_t key_len, 
                                  const uint8_t *label, uint32_t label_len, 
                                  const uint8_t *info, uint32_t info_len, 
                                  uint8_t *out, uint32_t out_len);

uint32_t onetls_hkdf_derive_finished(onetls_ctx *ctx, 
                                     const uint8_t *key, uint32_t key_len, 
                                     uint8_t *out, uint32_t out_len);

uint32_t onetls_hkdf_derive_key(onetls_ctx *ctx, 
                                const uint8_t *key, uint32_t key_len, 
                                uint8_t *out, uint32_t out_len);

uint32_t onetls_hkdf_derive_iv(onetls_ctx *ctx, 
                               const uint8_t *key, uint32_t key_len, 
                               uint8_t *out, uint32_t out_len);

uint32_t onetls_hkdf_derive_sn_key(onetls_ctx *ctx, 
                                   const uint8_t *key, uint32_t key_len, 
                                   uint8_t *out, uint32_t out_len);

uint32_t onetls_hkdf_derive_secret(onetls_ctx *ctx, 
                                   const uint8_t *label, uint32_t label_len,
                                   const uint8_t *secret, uint32_t secret_len,
                                   const uint8_t *in, uint32_t in_len,
                                   uint8_t *out, uint32_t out_len);   

uint32_t onetls_gen_early_secret(onetls_ctx *ctx);

uint32_t onetls_gen_secret(onetls_ctx *ctx, 
                           uint8_t *salt, 
                           uint32_t salt_len, 
                           uint8_t *in_secret, 
                           uint8_t *out_secret);     

uint32_t onetls_gen_traffic_secret_key_iv(onetls_ctx *ctx, 
                                          const char *label, 
                                          uint8_t *salt, uint32_t salt_len, 
                                          uint8_t *in_secret, 
                                          uint8_t *out_secret, 
                                          uint8_t *key, uint32_t key_len,
                                          uint8_t *iv, uint32_t iv_len,
                                          uint8_t *finish,
                                          uint8_t *sn_key);
#endif
																					