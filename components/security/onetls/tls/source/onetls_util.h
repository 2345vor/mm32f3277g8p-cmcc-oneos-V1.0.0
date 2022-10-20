#ifndef __ONETLS_UTIL_H__
#define __ONETLS_UTIL_H__
#include <string.h>
#include "onetls_client.h"

// 大小比较
#define onetls_min(a, b) ((a) > (b)) ? (b) : (a)
#define onetls_max(a, b) ((a) > (b)) ? (a) : (b)
#define onetls_abs(a, b) ((a) > (b)) ? ((a) - (b)) : ((b) - (a))

// 获取当前系统时间
uint32_t onetls_get_time(void);

uint64_t onetls_trans_seq_num(uint8_t *seq, uint32_t seq_size);

// seqnum 递增
void onetls_seq_num_add(uint8_t *seq, uint32_t seq_size);
void onetls_seq_num_reset(uint8_t *seq, uint32_t seq_size);

uint32_t onetls_get_random(onetls_ctx *ctx);
uint8_t onetls_tran_max_fragment_len();
uint32_t onetls_check_data_empty(const uint8_t *data, uint32_t len);
#endif
