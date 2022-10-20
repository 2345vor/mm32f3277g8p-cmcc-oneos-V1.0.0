#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* System support */
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_HAVE_TIME

/* mbed TLS modules */
/* 0. Entropy and Random */
// #define MBEDTLS_PSA_INJECT_ENTROPY
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_MD_C
#define MBEDTLS_HMAC_DRBG_C
/* 1. Hash */
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA512_C
/* 2. Cipher */
#define MBEDTLS_CIPHER_C
#define MBEDTLS_AES_C
#define MBEDTLS_CCM_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_POLY1305_C
#define MBEDTLS_CHACHAPOLY_C
/* 3. Key exchange */
#define MBEDTLS_ECP_C
#define MBEDTLS_ECDH_C
#define MBEDTLS_DHM_C
#define MBEDTLS_BIGNUM_C
/* 4. Curve */
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECP_DP_SECP384R1_ENABLED
#define MBEDTLS_ECP_DP_SECP521R1_ENABLED
#define MBEDTLS_ECP_DP_CURVE25519_ENABLED
#define MBEDTLS_ECP_DP_CURVE448_ENABLED

/* For test certificates */
// #define MBEDTLS_BASE64_C

/* PSA */
#define MBEDTLS_PSA_CRYPTO_C

#include "mbedtls/check_config.h"

#endif /* MBEDTLS_CONFIG_H */
