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
 * @file        a7670x_netconn.c
 *
 * @brief       a7670x module link kit netconnect api
 *
 * @revision
 * Date         Author          Notes
 * 2020-11-16   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include "a7670x_netconn.h"
#include "a7670x.h"
#include "mo_lib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DBG_EXT_TAG "a7670x.netconn"
#define DBG_EXT_LVL DBG_EXT_INFO
#include <os_dbg_ext.h>

#define SEND_DATA_MAX_SIZE        (1500)

#ifndef A7670X_DATA_QUEUE_SIZE
#define A7670X_DATA_QUEUE_SIZE    (5)
#endif

#define SET_EVENT(socket, event)  (((socket + 1) << 16) | (event))

#define A7670X_EVENT_NETCONN_OK   (1L << 0)
#define A7670X_EVENT_NETCONN_FAIL (1L << 1)

#ifdef A7670X_USING_NETCONN_OPS

enum net_transfer_mode
{
    NON_TRANSPARENT_MODE = 0,
    TRANSPARENT_MODE     = 1,
};

enum net_server_status
{
    NET_SERVER_DISABLE = 0,
    NET_SERVER_ENABLE  = 1,
};

enum netconn_send_status
{
    CONN_DISCONNECTED   = -1,
    CONN_SEND_BUFF_FULL = 0,
};

enum link_close_reason
{
    LINK_CLOSED_BY_LOCAL    = 0,
    LINK_CLOSED_BY_REMOTE   = 1,
    SEND_TIMEOUT_OR_DTR_OFF = 2,
    LINK_CLOSED_UNKNOWN,
};

static os_err_t a7670x_lock(os_mutex_t *mutex)
{
    return os_mutex_recursive_lock(mutex, OS_IPC_WAITING_FOREVER);
}

static os_err_t a7670x_unlock(os_mutex_t *mutex)
{
    return os_mutex_recursive_unlock(mutex);
}

static mo_netconn_t *a7670x_netconn_alloc(mo_object_t *module)
{
    mo_a7670x_t *a7670x = os_container_of(module, mo_a7670x_t, parent);

    for (int i = 0; i < A7670X_NETCONN_NUM; i++)
    {
        if (NETCONN_STAT_NULL == a7670x->netconn[i].stat)
        {
            /* TODO: Remove after adding the API of setting local port  */
            /* NOTE: A7670X module dosen't support auto allocate udp port */
            a7670x->netconn[i].local_port = i + 50000;
            a7670x->netconn[i].connect_id = i;
            return &a7670x->netconn[i];
        }
    }

    LOG_EXT_E("Moduel %s alloc netconn failed!", module->name);

    return OS_NULL;
}

static mo_netconn_t *a7670x_get_netconn_by_id(mo_object_t *module, os_int32_t connect_id)
{
    OS_ASSERT(OS_NULL != module);

    if (0 > connect_id || A7670X_NETCONN_NUM <= connect_id)
    {
        LOG_EXT_E("%s INVALID connet_id:[%d]!", __func__, connect_id);
        return OS_NULL;
    }

    mo_a7670x_t *a7670x = os_container_of(module, mo_a7670x_t, parent);

    for (int i = 0; i < A7670X_NETCONN_NUM; i++)
    {
        if (connect_id == a7670x->netconn[i].connect_id)
        {
            return &a7670x->netconn[i];
        }
    }

    return OS_NULL;
}

os_err_t a7670x_netconn_get_info(mo_object_t *module, mo_netconn_info_t *info)
{
    mo_a7670x_t *a7670x = os_container_of(module, mo_a7670x_t, parent);

    info->netconn_array = a7670x->netconn;
    info->netconn_nums  = sizeof(a7670x->netconn) / sizeof(a7670x->netconn[0]);

    return OS_EOK;
}

static os_err_t a7670x_netconn_set_transfer_mode(mo_object_t *module)
{
    at_parser_t *parser = &module->parser;
    os_err_t     result = OS_EOK;
    os_int32_t   mode   = -1;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = AT_RESP_TIMEOUT_DEF};

    result = at_parser_exec_cmd(parser, &resp, "AT+CIPMODE?");
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s get netconn transfer mode config fail", module->name);
        return OS_ERROR;
    }
    else
    {
        if (at_resp_get_data_by_kw(&resp, "+CIPMODE:", "+CIPMODE: %d", &mode) <= 0)
        {
            LOG_EXT_E("%s get netconn transfer mode config parse resp failed", module->name);
            return OS_ERROR;
        }
    }

    if (mode == TRANSPARENT_MODE)
    {
        result = at_parser_exec_cmd(parser, &resp, "AT+CIPMODE=0");
        if (OS_EOK != result)
        {
            LOG_EXT_E("Module %s set netconn non transparent mode fail", module->name);
            return OS_ERROR;
        }
    }
    else
    {
        LOG_EXT_D("Module %s netconn non transparent mode has been set up", module->name);
    }

    return result;
}

static os_err_t a7670x_netconn_server_open(mo_object_t *module)
{
    at_parser_t *parser = &module->parser;
    os_err_t     result = OS_EOK;
    os_int32_t   stat   = -1;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = AT_RESP_TIMEOUT_DEF};

    result = at_parser_exec_cmd(parser, &resp, "AT+NETOPEN?");
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s get netconn server status fail", module->name);
        return OS_ERROR;
    }
    else
    {
        if (at_resp_get_data_by_kw(&resp, "+NETOPEN:", "+NETOPEN: %d", &stat) <= 0)
        {
            LOG_EXT_E("%s get netconn server status parse resp failed", module->name);
            return OS_ERROR;
        }
    }

    if (stat == NET_SERVER_DISABLE)
    {
        result = at_parser_exec_cmd(parser, &resp, "AT+NETOPEN");
        if (OS_EOK != result)
        {
            LOG_EXT_E("Module %s open netconn server fail", module->name);
            return OS_ERROR;
        }
    }
    else
    {
        LOG_EXT_D("Module %s netconn server has been opened", module->name);
    }

    return result;
}

static os_err_t a7670x_net_server_config(mo_object_t *module)
{
    at_parser_t *parser = &module->parser;
    os_err_t     result = OS_EOK;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF] = {0};

    at_resp_t resp = {.buff = resp_buff, .buff_size = sizeof(resp_buff), .timeout = AT_RESP_TIMEOUT_DEF};

    /* Config 1: ip and port number information is not displayed when receiving data */
    result = at_parser_exec_cmd(parser, &resp, "AT+CIPSRIP=0");
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s set netconn recv data to don't displayed ip and port info fail", module->name);
        return OS_ERROR;
    }

    /* Config 2: add recv data header, the format is "+RECEIVE,<link num>,<data length>" */
    result = at_parser_exec_cmd(parser, &resp, "AT+CIPCCFG=10,0,0,1,1,0,500");
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s add recv data header fail", module->name);
        return OS_ERROR;
    }

    /* Config 3: select non-transparent mode, configure before Net server open */
    result = a7670x_netconn_set_transfer_mode(module);
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s select non-transparent mode fail", module->name);
        return OS_ERROR;
    }

    /* Config 4: the received data will be outputted to COM port directly by URC */
    result = at_parser_exec_cmd(parser, &resp, "AT+CIPRXGET=0");
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s set recv URC data automatically fail", module->name);
        return OS_ERROR;
    }

    /* Config 5: open net server */
    result = a7670x_netconn_server_open(module);
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s open net server fail", module->name);
        return OS_ERROR;
    }

    return OS_EOK;
}

mo_netconn_t *a7670x_netconn_create(mo_object_t *module, mo_netconn_type_t type)
{
    mo_a7670x_t *a7670x = os_container_of(module, mo_a7670x_t, parent);
    os_err_t     result = OS_EOK;

    a7670x_lock(&a7670x->netconn_lock);

    mo_netconn_t *netconn = a7670x_netconn_alloc(module);

    if (OS_NULL == netconn)
    {
        a7670x_unlock(&a7670x->netconn_lock);
        return OS_NULL;
    }

    /* Netconn server config */
    result = a7670x_net_server_config(module);
    if (OS_EOK != result)
    {
        LOG_EXT_E("Module %s netconn server config fail", module->name);
        a7670x_unlock(&a7670x->netconn_lock);
        return OS_NULL;
    }

    result = os_data_queue_init(&netconn->data_queue, A7670X_DATA_QUEUE_SIZE, 0, OS_NULL);
    if (result != OS_EOK)
    {
        LOG_EXT_E("%s data queue init failed, no enough memory.", module->name);
        a7670x_unlock(&a7670x->netconn_lock);
        return OS_NULL;
    }

    netconn->stat = NETCONN_STAT_INIT;
    netconn->type = type;

    a7670x_unlock(&a7670x->netconn_lock);

    return netconn;
}

os_err_t a7670x_netconn_destroy(mo_object_t *module, mo_netconn_t *netconn)
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
        result = at_parser_exec_cmd(parser, &resp, "AT+CIPCLOSE=%d", netconn->connect_id);
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

os_err_t a7670x_netconn_gethostbyname(mo_object_t *self, const char *domain_name, ip_addr_t *addr)
{
    at_parser_t *parser = &self->parser;

	char recvip[IPADDR_MAX_STR_LEN + 1] = {0};

    char resp_buff[256] = {0};

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .timeout   = 20 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser, &resp, "AT+CDNSGIP=\"%s\"", domain_name);
    if (result < 0)
    {
        result = OS_ERROR;
        goto __exit;
    }

    /* AT+CDNSGIP="www.qq.com" return: +CDNSGIP: 1,"www.qq.com","172.23.74.174"\r\n \r\n OK\r\n */
    if (at_resp_get_data_by_kw(&resp, "+CDNSGIP: 1", "+CDNSGIP: 1,%*[^,],\"%[^\"]\"", recvip) <= 0)
    {
        LOG_EXT_E("%s domain resolve: resp parse fail, try again, host: %s", self->name, domain_name);
        result = OS_ERROR;
        /* If resolve failed, maybe receive an URC CRLF */
        goto __exit;
    }

    if (strlen(recvip) < IPADDR_MIN_STR_LEN)
    {
        LOG_EXT_E("%s domain resolve: recvip len < IPADDR_MIN_STR_LEN, len = %d", self->name, strlen(recvip));
        result = OS_ERROR;
        goto __exit;
    }
    else
    {
        LOG_EXT_D("%s domain resolve: \"%s\" domain ip: %s, len %d", self->name, domain_name, recvip, strlen(recvip));
        inet_aton(recvip, addr);

        if (IPADDR_ANY == addr->addr || IPADDR_LOOPBACK == addr->addr)
        {
            ip_addr_set_zero(addr);
            result = OS_ERROR;
            goto __exit;
        }

        result = OS_EOK;
    }

__exit:

    return result;
}

static os_err_t a7670x_tcp_connect(mo_object_t *module, mo_netconn_t *netconn, char *ip_addr, os_uint16_t port)
{
    mo_a7670x_t *a7670x    = os_container_of(module, mo_a7670x_t, parent);
    at_parser_t *parser    = &module->parser;
    os_int32_t  connect_id = netconn->connect_id;
    os_err_t    result     = OS_ERROR;
    os_uint32_t  event     = SET_EVENT(netconn->connect_id, A7670X_EVENT_NETCONN_OK | A7670X_EVENT_NETCONN_FAIL);
    char resp_buff[64]     = {0};

    os_event_recv(&a7670x->netconn_evt, event, OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR, OS_IPC_WAITING_NO, OS_NULL);

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .line_num  = 0,
                      .timeout   = 30 * OS_TICK_PER_SECOND};

    a7670x_lock(&a7670x->netconn_lock);

    result = at_parser_exec_cmd(parser, &resp, "AT+CIPOPEN=%d,\"TCP\",\"%s\",%u", connect_id, ip_addr, port);
    if (result != OS_EOK)
    {
        goto __exit;
    }

    result = os_event_recv(&a7670x->netconn_evt,
                           SET_EVENT(connect_id, 0),
                           OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                           60 * OS_TICK_PER_SECOND,
                           OS_NULL);
    if (result != OS_EOK)
    {
        LOG_EXT_E("Module %s netconn id %d wait connect event timeout!", module->name, connect_id);
        goto __exit;
    }

    result = os_event_recv(&a7670x->netconn_evt,
                           A7670X_EVENT_NETCONN_OK | A7670X_EVENT_NETCONN_FAIL,
                           OS_EVENT_OPTION_OR | OS_EVENT_OPTION_CLEAR,
                           1 * OS_TICK_PER_SECOND,
                           &event);
    if (result != OS_EOK)
    {
        LOG_EXT_E("Module %s netconn id %d wait connect result timeout!", module->name, connect_id);
        goto __exit;
    }

    if (event & A7670X_EVENT_NETCONN_FAIL)
    {
        result = OS_ERROR;
        LOG_EXT_E("Module %s netconn id %d connect failed!", module->name, connect_id);
    }

__exit:

    a7670x_unlock(&a7670x->netconn_lock);

    return result;
}

static os_err_t a7670x_udp_connect(mo_object_t *module, mo_netconn_t *netconn)
{
    at_parser_t *parser = &module->parser;
    char resp_buff[64]  = {0};

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .line_num  = 0,
                      .timeout   = 30 * OS_TICK_PER_SECOND};

    os_err_t result = at_parser_exec_cmd(parser,
                                         &resp,
                                         "AT+CIPOPEN=%d,\"UDP\",,,%u",
                                         netconn->connect_id,
                                         netconn->local_port);
    if (result != OS_EOK)
    {
        LOG_EXT_E("AT cmd exec failed: AT+CIPOPEN=%d,\"UDP\",,,%u", netconn->connect_id, netconn->local_port);
    }

    /* URC connect func has processed the return message of this cmd, try to parse it is prohibited */

    return result;
}

os_err_t a7670x_netconn_connect(mo_object_t *module, mo_netconn_t *netconn, ip_addr_t addr, os_uint16_t port)
{
    os_err_t result = OS_EOK;

    char remote_ip[IPADDR_MAX_STR_LEN + 1] = {0};

    strncpy(remote_ip, inet_ntoa(addr), IPADDR_MAX_STR_LEN);

    switch (netconn->type)
    {
    case NETCONN_TYPE_TCP:
        result = a7670x_tcp_connect(module, netconn, remote_ip, port);
        break;

    case NETCONN_TYPE_UDP:
        result = a7670x_udp_connect(module, netconn);
        break;

    default:
        result = OS_ERROR;
        break;
    }

    if (result != OS_EOK)
    {
        LOG_EXT_E("Module %s connect to %s:%u failed!", module->name, remote_ip, port);
        return result;
    }

    ip_addr_copy(netconn->remote_ip, addr);
    netconn->remote_port = port;
    netconn->stat        = NETCONN_STAT_CONNECT;

    LOG_EXT_D("Module %s connect to %s:%u successfully!", module->name, remote_ip, port);

    return OS_EOK;
}

static os_size_t a7670x_tcp_udp_send(at_parser_t *parser, mo_netconn_t *netconn, const char *data, os_size_t size)
{
    os_err_t   result       = OS_EOK;
    os_size_t  sent_size    = 0;
    os_size_t  cur_pkt_size = 0;

    char resp_buff[AT_RESP_BUFF_SIZE_DEF]  = {0};
    char remote_ip[IPADDR_MAX_STR_LEN + 1] = {0};

    at_resp_t resp = {.buff      = resp_buff,
                      .buff_size = sizeof(resp_buff),
                      .line_num  = 2,
                      .timeout   = 30 * OS_TICK_PER_SECOND};

    strncpy(remote_ip, inet_ntoa(netconn->remote_ip), IPADDR_MAX_STR_LEN);

    /* Send the AT+CIPSEND cmd then receive the '>' response on the first line */
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

        switch (netconn->type)
        {
        case NETCONN_TYPE_TCP:
            result = at_parser_exec_cmd(parser, &resp, "AT+CIPSEND=%d,%d", netconn->connect_id, cur_pkt_size);
            if (result != OS_EOK)
            {
                LOG_EXT_E("Send TCP data cmd exec fail: AT+CIPSEND=%d,%d", netconn->connect_id, cur_pkt_size);
                goto __exit;
            }
            break;

        case NETCONN_TYPE_UDP:
            /* Set command to sent UDP data: AT+CIPSEND=1,64,"121.89.166.244",6589 */
            result = at_parser_exec_cmd(parser,
                                        &resp,
                                        "AT+CIPSEND=%d,%d,%s,%d",
                                        netconn->connect_id,
                                        cur_pkt_size,
                                        remote_ip,
                                        netconn->remote_port);
            if (result != OS_EOK)
            {
                LOG_EXT_E("Send UDP data cmd exec fail: AT+CIPSEND=%d,%d,%s,%d",
                          netconn->connect_id,
                          cur_pkt_size,
                          remote_ip,
                          netconn->remote_port);
                goto __exit;
            }
            break;

        default:
            result = OS_ERROR;
            LOG_EXT_E("Send data fail, netconn type error");
            goto __exit;
        }

        /* Protect the A7670X data sending process, prevent other threads to send AT commands */
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

os_size_t a7670x_netconn_send(mo_object_t *module, mo_netconn_t *netconn, const char *data, os_size_t size)
{
    at_parser_t *parser    = &module->parser;
    os_size_t    sent_size = 0;

    mo_a7670x_t *a7670x = os_container_of(module, mo_a7670x_t, parent);

    a7670x_lock(&a7670x->netconn_lock);

    sent_size = a7670x_tcp_udp_send(parser, netconn, data, size);

    a7670x_unlock(&a7670x->netconn_lock);

    return sent_size;
}

static void urc_tcp_connect_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    mo_object_t *module = os_container_of(parser, mo_object_t, parser);
    mo_a7670x_t *a7670x = os_container_of(module, mo_a7670x_t, parent);

    os_int32_t connect_id = -1;
    os_int32_t result     = -1;

    sscanf(data, "+CIPOPEN: %d,%d", &connect_id , &result);

    if (0 == result)
    {
        os_event_send(&a7670x->netconn_evt, SET_EVENT(connect_id, A7670X_EVENT_NETCONN_OK));
    }
    else
    {
        os_event_send(&a7670x->netconn_evt, SET_EVENT(connect_id, A7670X_EVENT_NETCONN_FAIL));
    }

    return;
}

static void urc_close_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    os_int32_t connect_id = -1;
    os_int32_t closed_by  = -1;

    sscanf(data, "+IPCLOSE: %d,%d", &connect_id, &closed_by);

    mo_object_t  *module  = os_container_of(parser, mo_object_t, parser);
    mo_netconn_t *netconn = a7670x_get_netconn_by_id(module, connect_id);

    if (OS_NULL == netconn)
    {
        LOG_EXT_E("Module %s recv close urc data of connect %d, but netconn has been destroyed", module->name, connect_id);
        return;
    }

    switch(closed_by)
    {
    case LINK_CLOSED_BY_LOCAL:
        LOG_EXT_D("Module %s connect %d, closed by local, active", module->name, connect_id);
        break;

    case LINK_CLOSED_BY_REMOTE:
        LOG_EXT_D("Module %s receive close urc data of connect %d, closed by remote", module->name, connect_id);
        break;

    case SEND_TIMEOUT_OR_DTR_OFF:
        LOG_EXT_D("Module %s receive close urc data of connect %d, sending timeout or DTR off",
                  module->name,
                  connect_id);
        break;

    default:
        /* add handler when we need */
        LOG_EXT_W("Module %s receive close urc data of connect %d, unknown reason [%d]",
                 module->name,
                 connect_id,
                 closed_by);
        break;
    }

    mo_netconn_pasv_close_notice(netconn);

    return;
}

static void urc_recv_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    os_int32_t connect_id = 0;
    os_int32_t data_size  = 0;
    os_int32_t timeout    = data_size > 10 ? data_size : 10;

    /* For ex: +RECEIVE,0,10\r\n 0123456789 */
    sscanf(data, "+RECEIVE,%d,%d,", &connect_id, &data_size);
    LOG_EXT_D("Moudle %s netconn %d receive %d bytes data", parser->name, connect_id, data_size);

    mo_object_t  *module  = os_container_of(parser, mo_object_t, parser);
    mo_netconn_t *netconn = a7670x_get_netconn_by_id(module, connect_id);
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

        /* Data context is not end with \r\n */
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

static void urc_send_msg_func(struct at_parser *parser, const char *data, os_size_t size)
{
    OS_ASSERT(OS_NULL != parser);
    OS_ASSERT(OS_NULL != data);

    os_int32_t connect_id   = -1;
    os_int32_t reqsend_len  = -1;
    os_int32_t cnfsend_len  = -2;
    os_err_t   result       = OS_EOK;

    /* +CIPSEND: 0,1500,1500\r\n */
    sscanf(data, "+CIPSEND: %d,%d,%d", &connect_id, &reqsend_len, &cnfsend_len);

    mo_object_t  *module  = os_container_of(parser, mo_object_t, parser);
    mo_netconn_t *netconn = a7670x_get_netconn_by_id(module, connect_id);

    if (OS_NULL == netconn)
    {
        LOG_EXT_E("Module %s recv send urc data of connect %d, but netconn has been destroyed", module->name, connect_id);
        return;
    }

    if (cnfsend_len == CONN_DISCONNECTED)
    {
        LOG_EXT_E("Module %s recv send urc data of connect %d, netconn is disconnected", module->name, connect_id);
        result = OS_ERROR;
    }
    else if (cnfsend_len == CONN_SEND_BUFF_FULL)
    {
        LOG_EXT_E("Module %s recv send urc data of connect %d, netconn send buffer is full", module->name, connect_id);
        result = OS_ERROR;
    }
    else if ((reqsend_len > 0) && (cnfsend_len != reqsend_len))
    {
        LOG_EXT_E("Module %s recv send urc data of connect %d, netconn send error, close this connect", module->name, connect_id);
        result = OS_ERROR;
    }
    else
    {
        LOG_EXT_E("Module %s recv send urc data of connect %d, data send OK", module->name, connect_id);
    }

    if (result == OS_ERROR)
    {
        mo_netconn_pasv_close_notice(netconn);
    }

    return;
}

static at_urc_t gs_urc_table[] = {
    {.prefix = "+CIPOPEN:",  .suffix = "\r\n", .func = urc_tcp_connect_func},
    {.prefix = "+IPCLOSE:",  .suffix = "\r\n", .func = urc_close_func},
    {.prefix = "+CIPSEND:",  .suffix = "\r\n", .func = urc_send_msg_func},
    {.prefix = "+RECEIVE,",  .suffix = "\r\n", .func = urc_recv_func},
};

void a7670x_netconn_init(mo_a7670x_t *module)
{
    /* Init module netconn array */
    memset(module->netconn, 0, sizeof(module->netconn));
    for (int i = 0; i < A7670X_NETCONN_NUM; i++)
    {
        module->netconn[i].connect_id = -1;
    }

    /* Set netconn urc table */
    at_parser_t *parser = &(module->parent.parser);
    at_parser_set_urc_table(parser, gs_urc_table, sizeof(gs_urc_table) / sizeof(gs_urc_table[0]));

    return;
}

#endif /* A7670X_USING_NETCONN_OPS */
