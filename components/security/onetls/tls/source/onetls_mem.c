#include "onetls_mem.h"
#include <stdlib.h>

#if (ONETLS_STACK_SIZE >= 8192)
static onetls_mem_mng g_onetls_mem_mng = { 0 };
static uint32_t g_onetls_free_ref_limit = ONETLS_STACK_SIZE / 64;

static uint32_t onetls_init_buf(void)
{
    if (ONETLS_STACK_SIZE % 4 != 0) {
        return ONETLS_MEM_INVALID_LEN;
    }

    // 生成第一个节点
    g_onetls_mem_mng.first = (onetls_mem_node *)(g_onetls_mem_mng.mem_buff);
    g_onetls_mem_mng.first->data_len = ONETLS_STACK_SIZE - sizeof(onetls_mem_node);
    g_onetls_mem_mng.first->magic_num = ONETLS_MEM_MAGIC_NOUSE;
    g_onetls_mem_mng.free_ref = 1;
    g_onetls_mem_mng.first->list.v_ptr = g_onetls_mem_mng.first;
    onetls_list_init(&(g_onetls_mem_mng.first->list));

    #ifdef ONETLS_SEC_MEM_CLEAN
        memset(g_onetls_mem_mng.first->data, 0, g_onetls_mem_mng.first->data_len);
    #endif
    return ONETLS_SUCCESS;
}

// 降低内存碎片
static void onetls_pullup_buf(void)    
{
    // TODO:
}

static void *onetls_malloc_buf(uint32_t size)
{
    uint32_t new_len = size + sizeof(onetls_mem_node);
    onetls_mem_node *node = g_onetls_mem_mng.first;
    onetls_mem_node *cur = NULL;

    if (g_onetls_mem_mng.free_ref > g_onetls_free_ref_limit) {
        onetls_pullup_buf();
    }

    do {
        // 第一步：先找到最后一个节点
        node = node->list.prev->v_ptr;

        // 第二步，从最后一个节点往前遍历，遇到没有使用的节点，做一些判断
        if (node->magic_num == ONETLS_MEM_MAGIC_NOUSE) {
            if (node->data_len == size) {   // 刚刚好一个内存分片的大小,直接就使用这个分片
                #if ONETLS_SEC_MEM_CLEAN
                    memset(node->data, 0, node->data_len);
                #endif
                node->magic_num = ONETLS_MEM_MAGIC_INUSE;
                return node->data;
            }
            if (node->data_len > new_len) { // 不能相等，容易产生空节点，空节点只能等着被吃掉，有余留还有可能能继续使用
                break;
            }
        }
    } while (node != g_onetls_mem_mng.first);

    if (node->data_len <= new_len) {    // 等于也不行，防止空节点，哪怕剩一个字节也好
        return NULL;
    }

    // 第三步：将节点分裂成两个节点
    node->data_len -= new_len;

    // 第四步：生成新的节点
    cur = (onetls_mem_node *)(node->data + node->data_len);

    // 第五步：将新节点加到当前节点的后面
    cur->data_len = size;
    cur->magic_num = ONETLS_MEM_MAGIC_INUSE;
    cur->list.v_ptr = cur;
    onetls_list_add(&(node->list), &(cur->list));

    #if ONETLS_SEC_MEM_CLEAN
        memset(cur->data, 0, cur->data_len);
    #endif
    return cur->data;
}

static void onetls_free_buf(void *ptr)
{
    // 第一步：获取内存节点
    onetls_mem_node *node = (onetls_mem_node *)(ptr - sizeof(onetls_mem_node)), *next = NULL, *prev = NULL;
    if ((node->magic_num != ONETLS_MEM_MAGIC_INUSE) || 
        (node->data_len == 0) ||
        (node->list.prev == NULL) ||
        (node->list.next == NULL)) {
        return;
    }

    #ifdef ONETLS_SEC_MEM_CLEAN
        memset(node->data, 0, node->data_len);
    #endif

    // 第二步：修改魔法数标识位
    node->magic_num = ONETLS_MEM_MAGIC_NOUSE;
    g_onetls_mem_mng.free_ref ++;

    // 第三步：消灭内存碎片
    
    // 3.1 检查后面一个节点是不是可以合并
    next = node->list.next->v_ptr;
    if ((next->magic_num == ONETLS_MEM_MAGIC_NOUSE) && (next != g_onetls_mem_mng.first)) {
        // 3.2 长度将其覆盖，吃掉他
        node->data_len += sizeof(onetls_mem_node) + next->data_len;

        // 3.3 删除后面的节点
        onetls_list_del(&(next->list));

        // 3.4 减引用
        g_onetls_mem_mng.free_ref --;
    }

    // 4.1 检查前面一个是不是可以合并
    prev = node->list.prev->v_ptr;
    if (prev->magic_num == ONETLS_MEM_MAGIC_NOUSE) {
        // 4.2 长度将其覆盖，被他吃掉
        prev->data_len += sizeof(onetls_mem_node) + node->data_len;

        // 4.3 将自己摘链
        onetls_list_del(&(node->list));

        // 4.4 减引用
        g_onetls_mem_mng.free_ref --;
    }
}

#else

static void *onetls_malloc_sys(uint32_t size)
{
    onetls_mem_node *ptr = (onetls_mem_node*)malloc(sizeof(onetls_mem_node) + size);

    #ifdef ONETLS_SEC_MEM_CLEAN
        memset(ptr, 0, sizeof(onetls_mem_node) + size);
    #endif

    ptr->data_len = size;
    ptr->magic_num = ONETLS_MEM_MAGIC_INUSE;
    onetls_list_init(&(ptr->list));
    return (void*)(ptr->data);
}

static void onetls_free_sys(void *p)
{
    onetls_mem_node *ptr = (onetls_mem_node *)((uint8_t*)p - sizeof(onetls_mem_node));

    #ifdef ONETLS_SEC_MEM_CLEAN
        memset(ptr->data, 0, ptr->data_len);
    #endif
    ptr->magic_num = ONETLS_MEM_MAGIC_NOUSE;
    free(ptr);
}

#endif

uint32_t onetls_mem_init(void)
{
#if (ONETLS_STACK_SIZE >= 8192)
    return onetls_init_buf();
#else
    return ONETLS_SUCCESS;
#endif
}

void *onetls_malloc(uint32_t size)
{
    uint8_t pack_len = size % 4;
    if ((size == 0) || (size > 0xffff)) {   // 限制在(0, 65535]
        return NULL;
    }    

#if (ONETLS_STACK_SIZE >= 8192)
    return onetls_malloc_buf((pack_len > 0) ? (size + 4 - pack_len) : size);
#else
    return onetls_malloc_sys((pack_len > 0) ? (size + 4 - pack_len) : size);
#endif
}

void onetls_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
#if (ONETLS_STACK_SIZE >= 8192)
    return onetls_free_buf(ptr);
#else
    return onetls_free_sys(ptr);
#endif
}
