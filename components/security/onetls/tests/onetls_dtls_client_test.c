/**
 ***********************************************************************************************************************
 * Copyright (c) 2021, China Mobile Communications Group Co.,Ltd.
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
 * @file        onetls_dtls_client_test.c
 *
 * @brief       onetls_dtls_client_test functions.
 *
 * @revision
 * Date         Author          Notes
 * 2021-03-16   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <stdio.h>
#include <sys/socket.h>
#include <onetls_lib.h>

/* Target server params */
#define TEST_TARGET_ADDR    "139.186.193.211"
#define TEST_TARGET_PORT    4433

/* PSK hint(or PSK Identity) */
static uint8_t test_psk_hint[] = {0x74, 0x65, 0x73, 0x74};
static uint8_t test_psk_key[] = {0xaa, 0xbb, 0xcc, 0xdd};

/* Test application data */
static uint8_t test_app_data[] = "Hello server!\r\n";

/**
 ***********************************************************************************************************************
 * @brief           onetls dtls client test
 *
 * @param[in]       none
 *
 * @return          -1: failed
 *                  0: success
 ***********************************************************************************************************************
 */
int onetls_dtls_client_test(void)
{
    int ret = 0;
    int sock = 0;
    struct sockaddr_in serv_addr;
    onetls_ctx *ctx = NULL;
    uint8_t buf[64] = {0};
    uint32_t send_len = 0;
    uint32_t recv_len = 0;

    /* Echo OneTLS version  */
    onetls_version_vrcb vrcb;
    os_kprintf("%s\r\n", onetls_version(&vrcb));

    /* Initialize OneTLS */
    ret = onetls_init();
    if (ret != 0) {
        os_kprintf("OneTLS initialized failed\r\n");
        return -1;
    }
    os_kprintf("OneTLS initialized sucessfully\r\n");

    /* Create a socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        os_kprintf("Create a socket failed\r\n");
        return -1;
    }
    os_kprintf("Create a socket sucessfully\r\n");

    /* Fill in the server address */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(TEST_TARGET_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(TEST_TARGET_ADDR);

    /* Connect to the server */
    ret = connect(sock, (void*)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        os_kprintf("Connect to server failed\r\n");
        return -1;        
    }
    os_kprintf("Connect to server sucessfully\r\n");

    /* Create a OneTLS context */
    ctx = onetls_ctx_new(ONETLS_MODE_DTLS);
    if (ctx == NULL) {
        os_kprintf("Create a OneTLS context failed\r\n");
        return -1;
    }
    os_kprintf("Create a OneTLS context sucessfully\r\n");

    /* Attach context to the socket */
    onetls_set_socket(ctx, sock, sock);

    /* Config psk hint */
    ret = onetls_set_outband_psk(ctx, test_psk_hint, sizeof(test_psk_hint), test_psk_key, sizeof(test_psk_key));
    if (ret != 0) {
        os_kprintf("Config psk hint failedr\r\n");
        return -1;
    }

    /* Config recv NST callback */
    // onetls_set_recv_ticket_cb(ctx, onetls_recv_new_session_ticket);

    /* Handshake with server side */
    while ((ret = onetls_connect(ctx)) != 0) {
        /* Only use in non-blocking socket */
        if ( ret != ONETLS_SOCKET_TRYAGAIN) {
            os_kprintf("Handshake with server failed, ret=%d\r\n", ret);
            goto cleanup;
        }
    }
    os_kprintf("Handshake with server sucessfully\r\n");

    /* Send and receive application data */
    ret = onetls_send(ctx, test_app_data, sizeof(test_app_data), &send_len);
    if (ret != 0) {
        os_kprintf("Send data to server failed\r\n");
        goto cleanup;
    }
    os_kprintf("Send data to server sucessfully\r\n");

    while ((ret = onetls_recv(ctx, buf, sizeof(buf), &recv_len)) == 0) {
        if (recv_len) break;
    }
    if (ret != 0) {
        os_kprintf("Recv data from server failed\r\n");
        goto cleanup;
    }
    os_kprintf("Recv data from server sucessfully\r\n");
    if (recv_len) {
        os_kprintf("Recv data from server[%d]:\r\n", recv_len);
        os_kprintf("%s\r\n", buf);
    }
    os_kprintf("Close the connection ...done\r\n"); 

cleanup:
    /* Shutdown the connect  */
    onetls_shutdown(ctx);

    /* Delete context */
    onetls_ctx_del(ctx);

    /* Close the socket */
    closesocket(sock);

    return ret;
}

#ifdef OS_USING_SHELL
#include <shell.h>
SH_CMD_EXPORT(dtls_test, onetls_dtls_client_test, "start onetls dtls1.3 client test");
#endif