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
 * @file        l610_netserv.c
 *
 * @brief       l610 module link kit netservice api
 *
 * @revision
 * Date         Author          Notes
 * 2020-11-12   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include "l610_netserv.h"
#include "l610.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MO_LOG_TAG "l610.netserv"
#define MO_LOG_LVL MO_LOG_INFO
#include <mo_log.h>

#ifdef L610_USING_NETSERV_OPS

os_err_t l610_netserv_open(mo_object_t *self)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_256] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 2 * OS_TICK_PER_SECOND};

    if (at_parser_exec_cmd(parser, &resp, "AT+COPS?") != OS_EOK)
    {
        ERROR("Module %s read operator selects failed", self->name);
        return OS_ERROR;
    }

    char oper[17] = {0};
    if (at_resp_get_data_by_kw(&resp, "+COPS:", "+COPS: %*d,%*d,\"%[^\"]\",%*d", oper) < 0)
    {
        WARN("Module %s parser operator selects failed", self->name);
        return OS_ERROR;
    }

    char apn[10] = {0};
    if (strcmp(oper, "CHINA MOBILE") == 0)
    {
        strncpy(apn, "CMNET", strlen("CMNET"));
    }
    else if (strcmp(oper, "CHN-UNICOM") == 0)
    {
        strncpy(apn, "3GNET", strlen("3GNET"));
    }
    else if (strcmp(oper, "CHN-CT") == 0)
    {
        strncpy(apn, "CTNET", strlen("CTNET"));
    }

    resp.line_num = 2;
    resp.timeout  = 150 * OS_TICK_PER_SECOND;

    if (at_parser_exec_cmd(parser, &resp, "AT+MIPCALL=1,\"%s\"", apn) != OS_EOK)
    {
        ERROR("Module %s open tcp/ip protocol stack failed", self->name);
        return OS_ERROR;
    }

    os_int32_t stat = 0;
    if ((at_resp_get_data_by_kw(&resp, "+MIPCALL:", "+MIPCALL: %d", &stat) <= 0) || (0 == stat))
    {
        WARN("Module %s open tcp/ip protocol stack, resp parse failed", self->name);
        return OS_ERROR;
    }

    INFO("Module %s netserv open OK", self->name);

    return OS_EOK;
}

os_err_t l610_set_attach(mo_object_t *self, os_uint8_t attach_stat)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 15 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+CGATT=%d", attach_stat);

    if (result != OS_EOK)
    {
        return result;
    }

    if (1 == attach_stat)
    {
        os_task_msleep(3000);

        result = l610_netserv_open(self);
        if (result != OS_EOK)
        {
            WARN("Module %s netserv open failed", self->name);
        }
    }

    return result;
}

os_err_t l610_get_attach(mo_object_t *self, os_uint8_t *attach_stat)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 3 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+CGATT?");
    if (result != OS_EOK)
    {
        return OS_ERROR;
    }

    if(at_resp_get_data_by_kw(&resp, "+CGATT:", "+CGATT:%hhu", attach_stat) <= 0)
    {
        ERROR("Get %s module attach state failed", self->name);
        return OS_ERROR;
    }

    return OS_EOK;
}

os_err_t l610_set_reg(mo_object_t *self, os_uint8_t reg_n)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 2 * OS_TICK_PER_SECOND};

    return at_parser_exec_cmd(parser, &resp, "AT+CEREG=%d", reg_n);
}

os_err_t l610_get_reg(mo_object_t *self, eps_reg_info_t *info)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_256] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 2 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+CEREG?");
    if (result != OS_EOK)
    {
        return OS_ERROR;
    }

    if (at_resp_get_data_by_kw(&resp, "+CEREG:", "+CEREG:%hhu,%hhu", &info->reg_n, &info->reg_stat) <= 0)
    {
        ERROR("Get %s module register state failed", self->name);
        return OS_ERROR;
    }

    return OS_EOK;
}

os_err_t l610_set_cgact(mo_object_t *self, os_uint8_t cid, os_uint8_t act_stat)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

     at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 30 * OS_TICK_PER_SECOND};

    return at_parser_exec_cmd(parser, &resp, "AT+CGACT=%d,%d", act_stat, cid);
}

os_err_t l610_get_cgact(mo_object_t *self, os_uint8_t *cid, os_uint8_t *act_stat)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

     at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 3 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+CGACT?");
    if (result != OS_EOK)
    {
        return OS_ERROR;
    }

    if (at_resp_get_data_by_kw(&resp, "+CGACT:", "+CGACT:%hhu,%hhu,%*s", cid, act_stat) <= 0)
    {
        ERROR("Get %s module cgact state failed", self->name);
        return OS_ERROR;
    }

    return OS_EOK;
}

os_err_t l610_get_csq(mo_object_t *self, os_uint8_t *rssi, os_uint8_t *ber)
{
    at_parser_t *parser = &self->parser;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

     at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = 2 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+CSQ?");
    if (result != OS_EOK)
    {
        return OS_ERROR;
    }

    if (at_resp_get_data_by_kw(&resp, "+CSQ:", "+CSQ:%hhu,%hhu", rssi, ber) <= 0)
    {
        ERROR("Get %s module signal quality failed", self->name);
        return OS_ERROR;
    }

    return OS_EOK;
}

#endif /* L610_USING_NETSERV_OPS */

