#ifndef __ONETLS_MEM_H__
#define __ONETLS_MEM_H__
#include "onetls_client.h"
#include "onetls_list.h"

#define ONETLS_MEM_MAGIC_INUSE (0xabababab)
#define ONETLS_MEM_MAGIC_NOUSE (0xcdcdcdcd)

typedef struct {    
    onetls_list_node list;  // 链表管理
    uint32_t magic_num;     // 魔法数的意义在于校验操作的合法性，原则上可以加上内存保护，做内存监控
    uint32_t data_len;      // 数据块长度
    uint8_t  data[0];       // 数据块
} onetls_mem_node;

#if (ONETLS_STACK_SIZE >= 8192)
    typedef struct {
        // 其实可以将free malloc拆分为两个链表。各自独立维护。想着我们对性能没有要求。感觉用不上
        onetls_mem_node *first; // 第一个内存分片
        
        uint32_t free_ref;  // free分片数量
        uint8_t mem_buff[ONETLS_STACK_SIZE];
    } onetls_mem_mng;
#endif

// 初始化内存接口
uint32_t onetls_mem_init(void);

// 通用内存申请释放接口
void *onetls_malloc(uint32_t size);
void  onetls_free(void *ptr);

#endif
