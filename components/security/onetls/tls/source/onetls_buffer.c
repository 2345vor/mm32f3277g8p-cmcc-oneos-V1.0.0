#include "onetls_buffer.h"
#include "onetls_mem.h"
#include "onetls_util.h"

struct st_st_onetls_buffer {
    uint32_t cursor;    // 游标
    uint32_t tail;      // 当前既有数据长度

    uint32_t size;      // 数据块总长度
    uint8_t  data[0];
};

onetls_buffer *onetls_buffer_new(uint32_t size)
{
    onetls_buffer *buffer = (onetls_buffer *)onetls_malloc(sizeof(onetls_buffer) + size);
    if (buffer == NULL) {
        return NULL;
    }
    memset(buffer, 0, sizeof(onetls_buffer));
    buffer->size = size;
    return buffer;
}

void onetls_buffer_del(onetls_buffer *buffer)
{
    if (buffer == NULL) return;
    onetls_free(buffer);
}

void onetls_buffer_del_safe(onetls_buffer **buffer)
{
    if ((buffer == NULL) || (*buffer == NULL)) {
        return;
    }
    onetls_buffer_del(*buffer);
    *buffer = NULL;
}

void onetls_buffer_reset(onetls_buffer *buffer)
{
    if (buffer == NULL) {
        return;
    }
    buffer->cursor = 0;
    buffer->tail = 0;

    #ifdef ONETLS_SEC_MEM_CLEAN
        memset(buffer->data, 0, buffer->size);
    #endif    
}

uint8_t *onetls_buffer_get_data(onetls_buffer *buffer)
{
    return buffer->data;
}

uint32_t onetls_buffer_get_cursor(onetls_buffer *buffer)
{
    return buffer->cursor;
}

uint8_t *onetls_buffer_get_cursor_p(onetls_buffer *buffer)
{
    return buffer->data + buffer->cursor;
}

uint32_t onetls_buffer_set_cursor(onetls_buffer *buffer, uint32_t new_cursor)
{
    buffer->cursor = onetls_min(new_cursor, buffer->tail);
    return buffer->cursor;
}

uint32_t onetls_buffer_setup_cursor(onetls_buffer *buffer, uint32_t up_cursor)
{
    uint32_t new_cursor = buffer->cursor + up_cursor;
    if (new_cursor > buffer->size) {
        return 0;
    }
    buffer->tail = onetls_max(buffer->tail, new_cursor);
    return onetls_buffer_set_cursor(buffer, new_cursor);
}

uint32_t onetls_buffer_setback_cursor(onetls_buffer *buffer, uint32_t back_cursor)
{
    uint32_t new_cursor = buffer->cursor - back_cursor;
    if (new_cursor > buffer->size) {
        return 0;
    }
    buffer->tail = onetls_max(buffer->tail, new_cursor);
    return onetls_buffer_set_cursor(buffer, new_cursor);
}

uint32_t onetls_buffer_set_tail(onetls_buffer *buffer, uint32_t new_tail)
{
    buffer->tail = onetls_min(new_tail, buffer->size);
    buffer->cursor = onetls_min(buffer->cursor, new_tail);
    return buffer->tail;
}

uint32_t onetls_buffer_setup_tail(onetls_buffer *buffer, uint32_t up_tail)
{
    uint32_t new_tail = buffer->tail + up_tail;
    return onetls_buffer_set_tail(buffer, new_tail);
}

uint32_t onetls_buffer_get_tail(onetls_buffer *buffer)
{
    return buffer->tail;
}

uint8_t *onetls_buffer_get_tail_p(onetls_buffer *buffer)
{
    return buffer->data + buffer->tail;
}

uint32_t onetls_buffer_cursor_to_tail(onetls_buffer *buffer)
{
    return buffer->tail - buffer->cursor;
}

uint32_t onetls_buffer_cursor_to_size(onetls_buffer *buffer)
{
    return buffer->size - buffer->cursor;
}

uint32_t onetls_buffer_get_uint8(onetls_buffer *buffer, uint8_t *b)
{
    *b = 0;
    if ((buffer->cursor + 1) > buffer->tail) {
        return 0;
    }
    *b = buffer->data[(buffer->cursor)++];
    return 1;
}

uint32_t onetls_buffer_get_uint16(onetls_buffer *buffer, uint16_t *s)
{
    uint16_t t = 0;
    if ((buffer->cursor + 2) > buffer->tail) {
        return 0;
    }
    *s = t;
    *s |= (t = buffer->data[(buffer->cursor)++]) << 8;
    *s |= (t = buffer->data[(buffer->cursor)++]);
    return 2;
}

uint32_t onetls_buffer_get_uint24(onetls_buffer *buffer, uint32_t *l)
{
    uint32_t t = 0;
    if ((buffer->cursor + 3) > buffer->tail) {
        return 0;
    }
    *l = t;
    *l |= (t = buffer->data[(buffer->cursor)++]) << 16;
    *l |= (t = buffer->data[(buffer->cursor)++]) << 8;
    *l |= (t = buffer->data[(buffer->cursor)++]);
    return 3;
}

uint32_t onetls_buffer_get_uint32(onetls_buffer *buffer, uint32_t *l)
{
    uint32_t t = 0;
    if ((buffer->cursor + 4) > buffer->tail) {
        return 0;
    } 
    *l = t;
    *l |= (t = buffer->data[(buffer->cursor)++]) << 24;
    *l |= (t = buffer->data[(buffer->cursor)++]) << 16;
    *l |= (t = buffer->data[(buffer->cursor)++]) << 8;
    *l |= (t = buffer->data[(buffer->cursor)++]);
    return 4; 
}

uint32_t onetls_buffer_get_uint48(onetls_buffer *buffer, uint64_t *ll)
{
    uint64_t t = 0;
    if ((buffer->cursor + 6) > buffer->tail) {
        return 0;
    } 
    *ll = t;
    *ll |= (t = buffer->data[(buffer->cursor)++]) << 40;
    *ll |= (t = buffer->data[(buffer->cursor)++]) << 32;
    *ll |= (t = buffer->data[(buffer->cursor)++]) << 24;
    *ll |= (t = buffer->data[(buffer->cursor)++]) << 16;
    *ll |= (t = buffer->data[(buffer->cursor)++]) << 8;
    *ll |= (buffer->data[(buffer->cursor)++]);
    return 6;     
}

uint32_t onetls_buffer_get_stream(onetls_buffer *buffer, uint8_t *data, uint32_t len)
{
    if ((buffer->cursor + len) > buffer->tail) {
        return 0;
    }
    memcpy(data, buffer->data + buffer->cursor, len);
    buffer->cursor += len;
    return len;
}

uint32_t onetls_buffer_get_all_data(onetls_buffer *buffer, uint8_t *data, uint32_t len)
{
    uint32_t real_len = onetls_min(len, buffer->tail);
    memcpy(data, buffer->data, real_len);
    return real_len;
}

uint32_t onetls_buffer_put_uint8(onetls_buffer *buffer, uint8_t b)
{
    if ((buffer->cursor + 1) > buffer->size) {    // 没得可以写的空间了
        return 0;
    }
    buffer->data[(buffer->cursor)++] = b;
    buffer->tail = onetls_max(buffer->tail, buffer->cursor);
    return 1;    
}

uint32_t onetls_buffer_put_uint16(onetls_buffer *buffer, uint16_t s)
{
    if ((buffer->cursor + 2) > buffer->size) {    // 没得可以写的空间了
        return 0;
    }
    buffer->data[(buffer->cursor)++] = (s & 0xff00) >> 8;
    buffer->data[(buffer->cursor)++] = (s & 0x00ff);
    buffer->tail = onetls_max(buffer->tail, buffer->cursor);
    return 2;
}

uint32_t onetls_buffer_put_uint24(onetls_buffer *buffer, uint32_t l)
{
    if ((buffer->cursor + 3) > buffer->size) {    // 没得可以写的空间了
        return 0;
    }
    buffer->data[(buffer->cursor)++] = (l & 0x00ff0000) >> 16;
    buffer->data[(buffer->cursor)++] = (l & 0x0000ff00) >> 8;
    buffer->data[(buffer->cursor)++] = (l & 0x000000ff);
    buffer->tail = onetls_max(buffer->tail, buffer->cursor);
    return 3;    
}

uint32_t onetls_buffer_put_uint32(onetls_buffer *buffer, uint32_t l)
{
    if ((buffer->cursor + 4) > buffer->size) {    // 没得可以写的空间了
        return 0;
    }
    buffer->data[(buffer->cursor)++] = (l & 0xff000000) >> 24;
    buffer->data[(buffer->cursor)++] = (l & 0x00ff0000) >> 16;
    buffer->data[(buffer->cursor)++] = (l & 0x0000ff00) >> 8;
    buffer->data[(buffer->cursor)++] = (l & 0x000000ff);
    buffer->tail = onetls_max(buffer->tail, buffer->cursor);
    return 4;
}

uint32_t onetls_buffer_put_uint48(onetls_buffer *buffer, uint64_t ll)
{
    if ((buffer->cursor + 6) > buffer->size) {    // 没得可以写的空间了
        return 0;
    }
    buffer->data[(buffer->cursor)++] = (ll & 0xff0000000000) >> 40;
    buffer->data[(buffer->cursor)++] = (ll & 0x00ff00000000) >> 32;
    buffer->data[(buffer->cursor)++] = (ll & 0x0000ff000000) >> 24;
    buffer->data[(buffer->cursor)++] = (ll & 0x000000ff0000) >> 16;
    buffer->data[(buffer->cursor)++] = (ll & 0x00000000ff00) >> 8;
    buffer->data[(buffer->cursor)++] = (ll & 0x0000000000ff);
    buffer->tail = onetls_max(buffer->tail, buffer->cursor);
    return 6;
}

uint32_t onetls_buffer_put_stream(onetls_buffer *buffer, const uint8_t *data, uint32_t len)
{
    if ((buffer->cursor + len) > buffer->size) {    // 没得可以写的空间了,考虑实际情况，我认为不会翻转，暂时不考虑
        return 0;
    }
    memcpy(buffer->data + buffer->cursor, data, len);
    buffer->cursor += len;
    buffer->tail = onetls_max(buffer->tail, buffer->cursor);
    return len;
}

uint32_t onetls_buffer_put_stream_realloc(onetls_buffer **buffer, const uint8_t *data, uint32_t len, uint32_t reserve)
{
    onetls_buffer *new_buffer = NULL;

    // 如果buffer是空的
    if (*buffer == NULL) {
        *buffer = onetls_buffer_new(len + reserve);
    } else if (((*buffer)->cursor + len) > (*buffer)->size) {
        // 新申请一块内存
        new_buffer = onetls_buffer_new((*buffer)->cursor + len + reserve);

        // 将老数据拷贝过来
        onetls_buffer_put_stream(new_buffer, onetls_buffer_get_data(*buffer), onetls_buffer_get_tail(*buffer));

        // 将原来的删除掉
        onetls_buffer_del(*buffer);
        (*buffer) = new_buffer;
    }
    return onetls_buffer_put_stream((*buffer), data, len);
}
