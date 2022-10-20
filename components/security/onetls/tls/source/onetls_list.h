#ifndef __ONETLS_LIST_H__
#define __ONETLS_LIST_H__
#include <stdint.h>

typedef struct st_onetls_list_node {
    struct st_onetls_list_node *prev;
    struct st_onetls_list_node *next;
    
    union {
        uint8_t     v_byte;
        uint16_t    v_word;
        uint32_t    v_dword;
        void*       v_ptr;
    };
} onetls_list_node;

static inline void onetls_list_init(onetls_list_node *node)
{
    node->next = node;
    node->prev = node;
}

static inline void onetls_list_add(onetls_list_node *head, onetls_list_node *node)
{
    node->next = head->next;
    node->prev = head;
    head->next->prev = node;
    head->next = node;
}

static inline void onetls_list_del(onetls_list_node *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    onetls_list_init(node);
}

#endif
