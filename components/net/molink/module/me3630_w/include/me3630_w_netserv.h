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
 * @file        me3630_w_netserv.h
 *
 * @brief       me3630_w module link kit netservice api declaration
 *
 * @revision
 * Date         Author          Notes
 * 2020-12-1   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#ifndef __ME3630_W_NETSERV_H__
#define __ME3630_W_NETSERV_H__

#include "mo_netserv.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef ME3630_W_USING_NETSERV_OPS

os_err_t me3630_w_set_attach(mo_object_t *self, os_uint8_t attach_stat);
os_err_t me3630_w_get_attach(mo_object_t *self, os_uint8_t *attach_stat);
os_err_t me3630_w_set_reg(mo_object_t *self, os_uint8_t reg_n);
os_err_t me3630_w_get_reg(mo_object_t *self, eps_reg_info_t *info);
os_err_t me3630_w_set_cgact(mo_object_t *self, os_uint8_t cid, os_uint8_t act_n);
os_err_t me3630_w_get_cgact(mo_object_t *self, os_uint8_t *cid, os_uint8_t *act_stat);
os_err_t me3630_w_get_csq(mo_object_t *self, os_uint8_t *rssi, os_uint8_t *ber);

#endif /* ME3630_W_USING_NETSERV_OPS */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ME3630_W_NETSERV_H__ */

