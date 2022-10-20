#ifndef __ONETLS_LOG_H__
#define __ONETLS_LOG_H__
#include "onetls_client.h"

// 输出二进制信息
#ifdef ONETLS_DEBUG
    void onetls_log_dump(const char *info, uint8_t *data, uint32_t data_len);
#else
    #define onetls_log_dump(a, b, c)
#endif

// 输出带错误码信息
void onetls_log_errcode(const char *file, const int line, const uint32_t err_code, const char *format, ...);

// 获取errno
int onetls_get_syscall_errno();

// check日志
#define onetls_check_errlog(err_code, format, ...) \
    do { \
        if ((err_code) != ONETLS_SUCCESS) { \
            onetls_log_errcode(__FILE__, __LINE__, (uint32_t)(err_code), format"\r\n", ##__VA_ARGS__); \
        } \
    } while (0)

// 断言
#ifdef ONETLS_ENABLE_ASSERT
    #define onetls_assert(c) \
        do { \
            if (!(c)) {\
                onetls_log_errcode(__FILE__, __LINE__, 0, "%s\r\n", "onetls_assert"); \
                while(1);\
            }\
        } while (0)
#else
    #define onetls_assert(c) 
#endif
#endif
