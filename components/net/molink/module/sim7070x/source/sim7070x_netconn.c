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
 * @file        sim7070x_netconn.c
 *
 * @brief       sim7070x module link kit netconnect api
 *
 * @revision
 * Date         Author          Notes
 * 2020-11-23   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include "sim7070x_netconn.h"
#include "sim7070x.h"
#include "mo_lib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBG_EXT_TAG "sim7070x.netconn"
#define DBG_EXT_LVL DBG_EXT_INFO
#include <os_dbg_ext.h>

#define SEND_DATA_MAX_SIZE        (1316)

#ifndef SIM7070X_DATA_QUEUE_SIZE
#define SIM7070X_DATA_QUEUE_SIZE    (5)
#endif

#ifdef SIM7070X_USING_NETCONN_OPS

enum pdpid0_init_status
{
    PDPIDX0_DISABLE = 0,
    PDPIDX0_ENABLE  = 1,
};

enum link_close_reason
{
    LINK_CLOSED_BY_REMOTE_OR_INTERNAL_ERR = 0,
    LINK_CONNECT_OK                       = 1,
    LINK_STATE_UNKNOWN,
};

static os_err_t sim7070x_lock(os_mutex_t *mutex)
{
    return os_mutex_recursive_lock(mutex, OS_IPC_WAITING_FOREVER);
}

static os_err_t sim7070x_unlock(os_mutex_t *mutex)
{
    return os_mutex_recursive_unlock(mutex);
}

static mo_netconn_t *sim7070x_netconn_alloc(mo_object_t *module)
{
    mo_sim7070x_t *sim7070x = os_container_of(module, mo_sim7070x_t, parent);

    for (int i = 0; i < SIM7070X_NETCONN_NUM; i++)
    {
        if (NETCONN_STAT_NULL == sim7070x->netconn[i].stat)
        {
            sim7070x->netconn[i].connect_id = i;
            return &sim7070x->netconn[i];
        }
    }

    LOG_EXT_E("Moduel %s alloc netconn failed!", module->name);

    return OS_NULL;
}

static mo_netconn_t *sim7070x_get_netconn_by_id(mo_object_t *module, os_int32_t connect_id)
{
    OS_ASSERT(OS_NULL != module);

    if ((connect_id < 0) || (connect_id > SIM7070X_NETCONN_NUM))
    {
        LOG_EXT_E("%s get netconn by id: INVALID connet_id: %d", module->name, connect_id);
        return OS_NULL;
    }

    mo_sim7070x_t *sim7070x = os_container_of(module, mo_sim7070x_t, parent);

    for (int i = 0; i < SIM7070X_NETCONN_NUM; i++)
    {
        if (connect_id == sim7070x->netconn[i].connect_id)
        {
            return &sim7070x->netconn[i];
        }
    }

    return OS_NULL;
}

os_err_t sim7070x_netconn_get_info(mo_object_t *module, mo_netconn_info_t *info)
{
    mo_sim7070x_t *sim7070x = os_container_of(module, mo_sim7070x_t, parent);

    info->netconn_array = sim7070x->netconn;
    info->netconn_nums  = sizeof(sim7070x->netconn) / sizeof(sim7070x->netconn[0]);

    return OS_EOK;
}

mo_netconn_t *sim7070x_netconn_create(mo_object_t *module, mo_netconn_type_t type)
{
    mo_sim7070x_t *sim7070x = os_container_of(module, mo_sim7070x_t, parent);
    os_err_t       result   = OS_EOK;

    sim7070x_lock(&sim7070x->netconn_lock);

    mo_netconn_t *netconn = sim7070x_netconn_alloc(module);

    if (OS_NULL == netconn)
    {
        sim7070x_unlock(&sim7070x->netconn_lock);
        return OS_NULL;
    }

    result = os_data_queue_init(&netconn->data_queue, SIM7070X_DATA_QUEUE_SIZE, 0, OS_NULL);
    if (result != OS_EOK)
    {
        LOG_EXT_E("%s data queue init failed, no enough memory.", module->name);
        sim7070x_unlock(&sim7070x->netconn_lock);
        return OS_NULL;
    }
    netconn->stat = NETCONN_STAT_INIT;
    netconn->type = type;

    sim7070x_unlock(&sim7070x->netconn_lock);

    return netconn;
}

os_err_t sim7070x_netconn_destroy(mo_object_t *module, mo_netconn_t *netconn)
{
    at_parser_t *parser = &module->parser;
    os_err_t     result = OS_ERROR;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = AT_RESP_TIMEOUT_DEF};

    LOG_EXT_I("Module %s in %d netconn status", module->name, netconn->stat);

    switch (netconn->stat)
    {
    case NETCONN_STAT_INIT:
        result = OS_EOK;
        break;

    case NETCONN_STAT_CONNECT:
    case NETCONN_STAT_CLOSE:
        result = at_parser_exec_cmd(parser, &resp, "AT+CACLOSE=%d", netconn->connect_id);
        if (result != OS_EOK)
        {
            LOG_EXT_E("Module %s destroy %s netconn failed",
                      module->name,
                      (netconn->type == NETCONN_TYPE_TCP) ? "TCP" : "UDP");
            return result;
        }
        break;

    default:
        /* add handler when we need */
        break;
    }

    if (netconn->stat != NETCONN_STAT_NULL)
    {
        mo_netconn_data_queue_deinit(&netconn->data_queue);
    }

    LOG_EXT_I("Module %s netconn_id: %d destroyed", module->name, netconn->connect_id);

    netconn->connect_id  = -1;
    netconn->stat        = NETCONN_STAT_NULL;
    netconn->type        = NETCONN_TYPE_NULL;
    netconn->remote_port = 0;
    netconn->local_port  = 0;
    inet_aton("0.0.0.0", &netconn->remote_ip);

    return result;
}

os_err_t sim7070x_netconn_gethostbyname(mo_object_t *self, const char *domain_name, ip_addr_t *addr)
{
    at_parser_t *parser = &self->parser;

	char recvip[IPADDR_MAX_STR_LEN + 1] = {0};

    char resp_buff[256] = {0};

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .line_num  = 4,
                      .timeout   = 20 * OS_TICK_PER_SECOND};

    /* Activate the app network 0th PDP first */
    os_err_t result = module_sim7070x_app_network_pdpidx0_init(self);
    if (result != OS_EOK)
    {
        return result;
    }

    result = at_parser_exec_cmd(parser, &resp, "AT+CDNSGIP=\"%s\",1,20000", domain_name);
    if (result != OS_EOK)
    {
        return result;
    }

    /* AT+CDNSGIP="www.qq.com",1,30000\r\n OK\r\n\r\n +CDNSGIP: 1,"www.qq.com","111.30.178.240"\r\n */
    if (at_resp_get_data_by_kw(&resp, "+CDNSGIP: 1", "+CDNSGIP: 1,%*[^,],\"%[^\"]\"", recvip) <= 0)
    {
        LOG_EXT_E("%s domain resolve: resp parse fail, try again, host: %s", self->name, domain_name);
        return OS_ERROR;
    }

    if (strlen(recvip) < IPADDR_MIN_STR_LEN)
    {
        LOG_EXT_E("%s domain resolve: recvip len < IPADDR_MIN_STR_LEN, len = %d", self->name, strlen(recvip));
        return OS_ERROR;
    }
    else
    {
        LOG_EXT_D("%s domain resolve: \"%s\" domain ip: %s, len %d", self->name, domain_name, recvip, strlen(recvip));
        inet_aton(recvip, addr);

        if (IPADDR_ANY == addr->addr || IPADDR_LOOPBACK == addr->addr)
        {
            ip_addr_set_zero(addr);
            return OS_ERROR;
        }

        result = OS_EOK;
    }

    return result;
}

os_err_t sim7070x_netconn_connect(mo_object_t *module, mo_netconn_t *netconn, ip_addr_t addr, os_uint16_t port)
{
    mo_sim7070x_t *sim7070x     = os_container_of(module, mo_sim7070x_t, parent);
    at_parser_t   *parser       = &module->parser;
    os_int32_t    connect_id    = netconn->connect_id;
    os_err_t      result        = OS_ERROR;
    char          resp_buff[64] = {0};

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .timeout   = 30 * OS_TICK_PER_SECOND};

    char remote_ip[IPADDR_MAX_STR_LEN + 1] = {0};

    strncpy(remote_ip, inet_ntoa(addr), IPADDR_MAX_STR_LEN);

    sim7070x_lock(&sim7070x->netconn_lock);
    result = at_parser_exec_cmd(parser, &resp, "AT+CASSLCFG=%d,\"SSL\",0", connect_id);
    if (result != OS_EOK)
    {
        goto __exit;
    }

    switch (netconn->type)
    {
    case NETCONN_TYPE_TCP:
        result = at_parser_exec_cmd(parser, &resp, "AT+CAOPEN=%d,0,\"TCP\",\"%s\",%u,1", connect_id, remote_ip, port);
        break;

    case NETCONN_TYPE_UDP:
        result = at_parser_exec_cmd(parser, &resp, "AT+CAOPEN=%d,0,\"UDP\",\"%s\",%u,1", connect_id, remote_ip, port);
        break;

    default:
        result = OS_ERROR;
        break;
    }

    ip_addr_copy(netconn->remote_ip, addr);
    netconn->remote_port = port;
    netconn->stat        = NETCONN_STAT_CONNECT;

    LOG_EXT_D("Module %s connect to %s:%u successfully!", module->name, remote_ip, port);

__exit:

    if (result != OS_EOK)
    {
        LOG_EXT_E("Module %s connect to %s:%u failed!", module->name, remote_ip, port);
    }

    sim7070x_unlock(&sim7070x->netconn_lock);

    return result;
}

static os_size_t sim7070x_tcp_udp_send(at_parser_t *parser, mo_netconn_t *netconn, const char *data, os_size_t size)
{
    os_err_t   result       = OS_EOK;
    os_size_t  sent_size    = 0;
    os_size_t  cur_pkt_size = 0;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF]  = {0};
    char remote_ip[IPADDR_MAX_STR_LEN + 1] = {0};

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .line_num  = 2,
                      .timeout   = 15 * OS_TICK_PER_SECOND};

    strncpy(remote_ip, inet_ntoa(netconn->remote_ip), IPADDR_MAX_STR_LEN);

    /* Send the AT+CASEND cmd then receive the '>' response on the first line */
    at_parser_set_end_mark(parser, ">", 1);

    while (sent_size < size)
    {
        /* Bytes to hex string(0x30 -> "30"), SEND_DATA_MAX_SIZE */
        if (size - sent_size < SEND_DATA_MAX_SIZE)
        {
            cur_pkt_size = size - sent_size;
        }
        else
        {
            cur_pkt_size = SEND_DATA_MAX_SIZE;
        }

        result = at_parser_exec_cmd(parser, &resp, "AT+CASEND=%d,%d,10000", netconn->connect_id, cur_pkt_size);
        if (result != OS_EOK)
        {
            LOG_EXT_E("Send data cmd exec fail: AT+CASEND=%d,%d,10000", netconn->connect_id, cur_pkt_size);
            goto __exit;
        }

        /* Protect the data sending process, prevent other threads to send AT commands */
        at_parser_exec_lock(parser);

        /* Sending the specified length of data context */
        if (at_parser_send(parser, data + sent_size, cur_pkt_size) <= 0)
        {
            result = OS_ERROR;
            goto __exit;
        }

        at_parser_exec_unlock(parser);
        sent_size += cur_pkt_size;
    }

__exit:

    /* Reset the end sign for data */
    at_parser_set_end_mark(parser, OS_NULL, 0);

    if (result != OS_EOK)
    {
        LOG_EXT_E("Module %s netconn %d send %d bytes data failed!",
                  parser->name,
                  netconn->connect_id,
                  cur_pkt_size);

        at_parser_exec_unlock(parser);
    }

    return sent_size;
}

os_size_t sim7070x_netconn_send(mo_object_t *module, mo_netconn_t *netconn, const char *data, os_size_t size)
{
    at_parser_t *parser    = &module->parser;
    os_size_t    sent_size = 0;

    mo_sim7070x_t *sim7070x = os_container_of(module, mo_sim7070x_t, parent);

    if (netconn->stat == NETCONN_STAT_CONNECT)
    {
        sim7070x_lock(&sim7070x->netconn_lock);

        sent_size = sim7070x_tcp_udp_send(parser, netconn, data, size);

        sim7070x_unlock(&sim7070x->netconn_lock);
    }
    else
    {
        LOG_EXT_E("Module %s netconn %d send %d bytes data failed, netconn state [%d] error",
                  parser->name,
                  netconn->connect_id,
                  size,
                  netconn->stat);
    }

    return sent_size;
}

static void urc_close_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    os_int32_t connect_id = -1;
    os_int32_t state      = -1;

    /* +CASTATE: <cid>,<state> */
    sscanf(data, "+CASTATE: %d,%d", &connect_id, &state);

    mo_object_t  *module  = os_container_of(parser, mo_object_t, parser);
    mo_netconn_t *netconn = sim7070x_get_netconn_by_id(module, connect_id);

    if (OS_NULL == netconn)
    {
        LOG_EXT_E("Module %s recv link state urc msg of connect %d, but netconn is not exist",
                  module->name,
                  connect_id);
        return;
    }

    switch (state)
    {
    case LINK_CLOSED_BY_REMOTE_OR_INTERNAL_ERR:
        LOG_EXT_D("Module %s connect %d is closed by remote server or internal error",
                  module->name,
                  connect_id);
        mo_netconn_pasv_close_notice(netconn);
        break;

    case LINK_CONNECT_OK:
        LOG_EXT_D("Module %s connect %d connect to remote server OK", module->name, connect_id);
        break;

    default:
        LOG_EXT_E("Module %s recv link state urc msg of connect %d, but state is unkonwn",
                  module->name,
                  connect_id);
        break;
    }

    return;
}

static void urc_recv_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    os_int32_t connect_id = 0;
    os_int32_t data_size  = 0;
    os_int32_t timeout    = data_size > 10 ? data_size : 10;

    /* For ex: +CAURC: "recv",0,10\r\n 0123456789 */
    sscanf(data, "+CAURC: \"recv\",%d,%d", &connect_id, &data_size);
    LOG_EXT_D("Moudle %s netconn %d receive %d bytes data", parser->name, connect_id, data_size);

    mo_object_t  *module  = os_container_of(parser, mo_object_t, parser);
    mo_netconn_t *netconn = sim7070x_get_netconn_by_id(module, connect_id);
    if (OS_NULL == netconn)
    {
        LOG_EXT_E("Module %s receive error recv urc data of connect %d", module->name, connect_id);
        return;
    }

    if (netconn->stat == NETCONN_STAT_CONNECT)
    {
        char *recv_buff    = calloc(1, data_size + 1);
        char  temp_buff[8] = {0};

        if (recv_buff == OS_NULL)
        {
            /* read and clean the coming data */
            LOG_EXT_E("Calloc recv buff %d bytes fail, no enough memory", (data_size + 1));
            os_size_t temp_size = 0;

            while (temp_size < data_size)
            {
                if (data_size - temp_size > sizeof(temp_buff))
                {
                    at_parser_recv(parser, temp_buff, sizeof(temp_buff), timeout);
                }
                else
                {
                    at_parser_recv(parser, temp_buff, data_size - temp_size, timeout);
                }

                temp_size += sizeof(temp_buff);
            }

            return;
        }

        if (at_parser_recv(parser, recv_buff, data_size, timeout) != data_size)
        {
            LOG_EXT_E("Module %s netconn %d recv %d bytes data fail", module->name, netconn->connect_id, data_size);
            return;
        }

        mo_netconn_data_recv_notice(netconn, recv_buff, data_size);
    }
    else
    {
        LOG_EXT_E("Moudle %s netconn id %d, status is not NETCONN_STAT_CONNECT", parser->name, connect_id);
    }

    return;
}

static at_urc_t gs_urc_table[] = {
    {.prefix = "+CASTATE:",         .suffix = "\r\n", .func = urc_close_func},
    {.prefix = "+CAURC: \"recv\"",  .suffix = "\r\n", .func = urc_recv_func},
};

void sim7070x_netconn_init(mo_sim7070x_t *module)
{
    /* Init module netconn array */
    memset(module->netconn, 0, sizeof(module->netconn));
    for (int i = 0; i < SIM7070X_NETCONN_NUM; i++)
    {
        module->netconn[i].connect_id = -1;
    }

    /* Set netconn urc table */
    at_parser_t *parser = &(module->parent.parser);
    at_parser_set_urc_table(parser, gs_urc_table, sizeof(gs_urc_table) / sizeof(gs_urc_table[0]));

    return;
}

#endif /* SIM7070X_USING_NETCONN_OPS */
