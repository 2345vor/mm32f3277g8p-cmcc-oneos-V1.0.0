/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        rcvr_object.h
 *
 * @brief       gnss recevicer object definition and api declaration
 *
 * @revision
 * Date         Author          Notes
 * 2020-12-14   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#ifndef __RCVR_OBJECT_H__
#define __RCVR_OBJECT_H__

#include <os_assert.h>
#include <os_memory.h>
#include <device.h>
#include <onepos_common.h>
#include "rcvr_type.h"

#ifdef ONEPOS_USING_GNSS_RCVR

#include <os_mutex.h>

struct rcvr_object;
typedef os_bool_t (*rcvr_prot_cb)(const os_int8_t *data, void* pos_data);
typedef struct {
    void        *pos_data;
    rcvr_prot_cb cb_func;
}rcvr_prot_t;

#ifdef RVCR_USING_PROT_PARSER
typedef struct{
    os_task_t  *task;
    rcvr_prot_t prot_table[RCVR_PROT_MAX]; 
}rcvr_prot_parse_t;
#endif
typedef os_err_t (*rcvr_ops_pfunc)(struct rcvr_object* rcvr, ...);

/**
 ***********************************************************************************************************************
 * @struct      rcvr_object_t
 *
 * @brief       gnss receiver object
 ***********************************************************************************************************************
 */
typedef struct rcvr_object
{
    os_slist_node_t     list;                     /* gnss receiver object manage list  */
    char                name[OS_NAME_MAX + 1];    /* gnss receiver object name */   
    os_device_t        *device;                   /* The device used by gnss receiver */
    os_mutex_t          op_lock;                  /* The recursive lock that protects the operate of the gnss recevicer */
	#ifdef RVCR_USING_PROT_PARSER
    rcvr_prot_parse_t  *prot_parser;              /* gnss receiver object using protocol table */
	#endif
    rcvr_ops_pfunc      ops_table[RCVR_OPS_MAX];  /* gnss receiver object operates table */
}rcvr_object_t;

extern os_err_t rcvr_object_op_lock(rcvr_object_t *rcvr);
extern os_err_t rcvr_object_op_unlock(rcvr_object_t *rcvr);

extern rcvr_object_t *rcvr_object_get_by_name(const char *name);
extern rcvr_object_t *rcvr_object_get_default(void);
extern void rcvr_object_set_default(rcvr_object_t *self);
extern os_err_t rcvr_object_init(rcvr_object_t *self, const char *name, const char* device_name);
extern os_err_t rcvr_object_deinit(rcvr_object_t *rcvr);
extern os_err_t rcvr_object_send(rcvr_object_t *rcvr, const char* data, os_size_t data_len);
extern os_err_t rcvr_object_read(rcvr_object_t *rcvr, char* dst, os_size_t size, os_uint32_t time_out);
extern void rcvr_object_destroy(rcvr_object_t *rcvr);
#ifdef RCVR_USING_AGNSS_OPS
extern os_err_t rcvr_agnss(rcvr_object_t *rcvr, double lat, double lon);
#endif
#ifdef RCVR_USING_RESET_OPS
extern os_err_t rcvr_reset(rcvr_object_t *rcvr, rcvr_reset_type_t reset_type);
#endif
#ifdef RVCR_USING_PROT_PARSER
extern os_err_t get_rcvr_data(rcvr_object_t *rcvr, void *src, os_size_t src_size, rcvr_prot_type_t prot_type);
#endif
#endif /* ONEPOS_USING_GNSS_RCVR */

#endif /* __RCVR_OBJECT_H__*/

