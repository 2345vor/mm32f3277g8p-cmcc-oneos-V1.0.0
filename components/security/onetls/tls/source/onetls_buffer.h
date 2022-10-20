#ifndef __ONETLS_BUFFER_H__
#define __ONETLS_BUFFER_H__
#include "onetls_client.h"

typedef struct st_st_onetls_buffer onetls_buffer;

// 申请一个buffer块
onetls_buffer *onetls_buffer_new(uint32_t size);
void onetls_buffer_reset(onetls_buffer *buffer);
void onetls_buffer_del(onetls_buffer *buffer);
void onetls_buffer_del_safe(onetls_buffer **buffer);

// 获取数据指针
uint8_t *onetls_buffer_get_data(onetls_buffer *buffer);
uint32_t onetls_buffer_get_all_data(onetls_buffer *buffer, uint8_t *data, uint32_t len);

// 游标操作
uint32_t onetls_buffer_get_cursor(onetls_buffer *buffer);
uint8_t *onetls_buffer_get_cursor_p(onetls_buffer *buffer);
uint32_t onetls_buffer_set_cursor(onetls_buffer *buffer, uint32_t new_cursor);
uint32_t onetls_buffer_setup_cursor(onetls_buffer *buffer, uint32_t up_cursor);
uint32_t onetls_buffer_setback_cursor(onetls_buffer *buffer, uint32_t back_cursor);

// 尾巴操作
uint32_t onetls_buffer_set_tail(onetls_buffer *buffer, uint32_t new_tail);
uint32_t onetls_buffer_setup_tail(onetls_buffer *buffer, uint32_t up_tail);
uint32_t onetls_buffer_get_tail(onetls_buffer *buffer);
uint8_t *onetls_buffer_get_tail_p(onetls_buffer *buffer);

// 获取剩余有效数据长度
uint32_t onetls_buffer_cursor_to_tail(onetls_buffer *buffer);
uint32_t onetls_buffer_cursor_to_size(onetls_buffer *buffer);

// buffer读取操作
uint32_t onetls_buffer_get_uint8(onetls_buffer *buffer, uint8_t *b);
uint32_t onetls_buffer_get_uint16(onetls_buffer *buffer, uint16_t *s);
uint32_t onetls_buffer_get_uint24(onetls_buffer *buffer, uint32_t *l);
uint32_t onetls_buffer_get_uint32(onetls_buffer *buffer, uint32_t *l);
uint32_t onetls_buffer_get_uint48(onetls_buffer *buffer, uint64_t *ll);
uint32_t onetls_buffer_get_stream(onetls_buffer *buffer, uint8_t *data, uint32_t len);

// put操作
uint32_t onetls_buffer_put_uint8(onetls_buffer *buffer, uint8_t b);
uint32_t onetls_buffer_put_uint16(onetls_buffer *buffer, uint16_t s);
uint32_t onetls_buffer_put_uint24(onetls_buffer *buffer, uint32_t l);
uint32_t onetls_buffer_put_uint32(onetls_buffer *buffer, uint32_t l);
uint32_t onetls_buffer_put_uint48(onetls_buffer *buffer, uint64_t ll);
uint32_t onetls_buffer_put_stream(onetls_buffer *buffer, const uint8_t *data, uint32_t len);
uint32_t onetls_buffer_put_stream_realloc(onetls_buffer **buffer, const uint8_t *data, uint32_t len, uint32_t reserve);

#endif

