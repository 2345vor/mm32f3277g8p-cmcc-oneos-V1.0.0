#ifndef __ONETLS_CONFIG_H__
#define __ONETLS_CONFIG_H__
#include <string.h>
#include <stdint.h>

#define ONETLS_EMBED_ONEOS

// 集成OneOS
#ifdef ONETLS_EMBED_ONEOS
#include <oneos_config.h>
#else
// 打开dump开关
#define ONETLS_ENABLE_DUMP

// 使用无堆操作、全部在栈上管理内存，定义无堆内存时的大小，不能低于4096
// #define ONETLS_HEAP_SIZE (40960)

// 最大报文长度，要包含record 11字节，tag 16字节，mt 1字节，所以一般多给32字节
#define ONETLS_MAX_PACKET_LEN  (1024)

// 使用基于PSA规范的加解密接口
#define ONETLS_USING_PSA

// 使用基于posix标准的IO接口
#define ONETLS_POSIX_IO

// 打开日志开关
#define ONETLS_ENABLE_LOG

// 配置单条日志最大长度
#define ONETLS_LOG_STRING_MAX_LEN (255)

// 打开断言开关
#define ONETLS_ENABLE_ASSERT

// socket超时时间默认为时间, 毫秒，0为不等待
#define ONETLS_SOCKET_WAIT_TIME 0xffffffff

// 曲线组
// #define ONETLS_ENABLE_ECDHE_SECP256R1
// #define ONETLS_ENABLE_ECDHE_SECP384R1
// #define ONETLS_ENABLE_ECDHE_SECP521R1
#define ONETLS_ENABLE_ECDHE_X25519   
// #define ONETLS_ENABLE_ECDHE_X448  // @TODO PSA_ERROR_NOT_SUPPORTED   

// 算法套件
#define ONETLS_ENABLE_TLS_AES_128_GCM_SHA256
// #define ONETLS_ENABLE_TLS_AES_256_GCM_SHA384 // 不能用于psk模式的完整握手
// #define ONETLS_ENABLE_TLS_CHACHA20_POLY1305_SHA256
// #define ONETLS_ENABLE_TLS_AES_128_CCM_SHA256
// #define ONETLS_ENABLE_TLS_AES_128_CCM_8_SHA256 

#endif

#endif
