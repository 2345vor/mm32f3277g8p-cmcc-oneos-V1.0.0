#include <stdarg.h>
#include <errno.h>
#include "onetls_log.h"
#include <stdio.h>

static void onetls_log_info(const char *info)
{
#if defined ONETLS_DEBUG
    printf("%s", info);
#endif
}

#ifdef ONETLS_DEBUG
void onetls_log_dump(const char *info, uint8_t *data, uint32_t data_len)
{

    uint32_t offset = 0;
    char log_string[ONETLS_LOG_STRING_MAX_LEN + 1] = { 0 };

    snprintf(log_string, ONETLS_LOG_STRING_MAX_LEN, "%-32s  data_len:%d\r\n", info, data_len);
    onetls_log_info(log_string);

    for (offset = 1; offset <= data_len; offset ++) {
        memset(log_string, 0, sizeof(log_string));
        snprintf(log_string, ONETLS_LOG_STRING_MAX_LEN, "0x%02x   ", data[offset - 1]);
        onetls_log_info(log_string);       

        // 换行
        if (offset % 8 == 0) onetls_log_info("\r\n");
    }
    onetls_log_info("\r\n\r\n");
}
#endif

void onetls_log_errcode(const char *file, const int line, const uint32_t err_code, const char *format, ...)
{   
    char log_string[ONETLS_LOG_STRING_MAX_LEN + 1] = { 0 };
    int len = 0;

    // 有兴趣的同学可以把时间戳加上
    len = snprintf(log_string, ONETLS_LOG_STRING_MAX_LEN, "%s_%d ret:0x%x errno:%d ", file, line, err_code, onetls_get_syscall_errno());
    if ((len <= 0) || (len >= ONETLS_LOG_STRING_MAX_LEN)) return;

    va_list vArgList;
    va_start(vArgList, format);
    (void)vsnprintf(log_string + len, ONETLS_LOG_STRING_MAX_LEN - len - 1, format, vArgList);
    va_end(vArgList);

    onetls_log_info(log_string);    
}

// 获取errno
int onetls_get_syscall_errno()
{    
    return errno;
}