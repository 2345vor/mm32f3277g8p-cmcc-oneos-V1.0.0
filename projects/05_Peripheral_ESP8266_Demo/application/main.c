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
 * @file        main.c
 *
 * @brief       User application entry
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <board.h>
#include <timer/clocksource.h>

#include "mo_common.h"
#include "esp8266.h"

// *************************** ����˵�� ***************************
// 
// ������Ҫ׼�� <OneOS ���￪����> һ��
// �Ƽ� MDK �汾Ϊ MDK5.28a
// 
// ����������Ҫ׼�� <��ɿƼ� CMSIS-DAP ����������> �� <ARM ����������> һ��
// ����ʹ�� <��׼ Jlink> �������ص����������
// 
// ȷ���������� �ϵ� ��ʹ�õ������������� ���Ƽ�ʹ�� USB���� ��ͨ��CH340��Type-C���й���
// ��¼�����̺� ���ʹ�õ��� <��ɿƼ� CMSIS-DAP ����������> �� <ARM ����������>
// ����ֱ���ڴ��������д򿪶�Ӧ����
// ���ʹ�õĲ����� <��ɿƼ� CMSIS-DAP ����������> �� <ARM ����������>
// ����� CH340 ��ʶ��Type-C�ӿڵ�����USB �ٴ򿪴��ڵ������ִ򿪶�Ӧ����
// 
// ������ʾ���� ��������ESP8266WIFIģ��ͨ��Molink�����ʼ��
// �������IP��ַ ����Ping���� 
// Ȼ�󹹽� TCP ���� ���Ҷ�Զ�˵�ַ�����շ����ݲ���
// �����Ϣͨ�� shell ���
// 
// ��Ҫ�������� 
// OneOS-Cube->Components->Network->Molink->Enable IoT Modules->WiFi Modules Support->ESP8266->ESP8266 Config
// ESP8266 Connect AP SSID Ϊ���Ի����� WiFi ����/SSID
// ESP8266 Connect AP Password Ϊ���Ի����� WiFi ����/Password
// 
// ��������Ҫһ������ʹ��������Թ��ܵĵ�������
// ����ʹ�� sscom �˿�����Ϊ TCP_Server �������ݵ�ַ�����·� REMOTE_IP
// �˿�Ĭ������Ϊ 8080 �˿�
// ������ɺ�������׼���ò���
// 
// ȷ��׼����ɺ���¼����ʼ���в���
// 
// ���µĹ��̻��߹����ƶ���λ�����ִ�����²���
// ��һ�� �ر��������д򿪵��ļ�
// �ڶ��� project->clean  �ȴ��·�����������
// 
// 2021-08-12	SEEKFREE
// 
// *************************** ����˵�� ***************************

#define PING_IP				"192.168.2.1"											// PING ���ԵĶ����ַ �����������Ϊ���� ���ݲ��������޸�
#define REMOTE_IP			"192.168.2.16"											// Զ��ͨ�ŵ�ַ ��������Ϊ����ʱʹ�õĵĵ��Ե�IP
#define REMOTE_PORT			8080													// ͨ�Ŷ˿�

static void user_task(void *parameter)
{
	os_err_t molink_result = OS_ERROR;												// ִ��״̬����

	mo_object_t *molink_modle = mo_get_by_name(ESP8266_NAME);						// ��ȡ ESP8266 ����
	OS_ASSERT(molink_modle != OS_NULL);												// ȷ�϶���ʵ������

	char molink_data_buffer[16];													// �½�һ��������������� IP ��ַ
	molink_result = mo_get_ipaddr(molink_modle, molink_data_buffer);				// ��ȡ����� IP ��ַ
	if(molink_result != OS_EOK)														// �жϻ�ȡ�Ƿ�ɹ�
	{
		LOG_W("mo_get_ipaddr", "get ipaddr faild.");								// ��ȡ��ַʧ�� ���������Ϣ
	}
	else
	{
		LOG_I("mo_get_ipaddr", "IP:%s.", molink_data_buffer);						// ��ȡ��ַ�ɹ� ��� IP ��ַ��Ϣ
	}

	ping_resp_t ping_test_t;														// �½� ping ���Խṹ��
	molink_result = mo_ping(molink_modle, PING_IP, 64, 10, &ping_test_t);			// ping ����
	if(molink_result != OS_EOK)														// �ж��Ƿ���Գɹ�
	{
		LOG_W("mo_ping", "ping faild.");											// ping ����ʧ�� ���������Ϣ
	}
	else
	{
		LOG_I("mo_ping", "PING IP:%s | LEN:%d | TIME OUT:%d | TIME RESPONSE:%d.", 	// ping ���Գɹ� ��� ping �����Ϣ
			inet_ntoa(ping_test_t.ip_addr.addr),									// �� IP ������ģʽת��Ϊ�ַ���
			ping_test_t.data_len,													// ��ʾ ping ���Ե����ݳ���
			ping_test_t.ttl,														// ��ʾ��ʱʱ��
			ping_test_t.time);														// ��ʾ�ӳ�ʱ��
	}

	mo_netconn_t *test_netconn = mo_netconn_create(molink_modle, NETCONN_TYPE_TCP);	// �½����� netceonn �Ķ���ʵ�� ��ʽΪ TCP
	if(test_netconn == OS_NULL)														// �ж��Ƿ��½�ʵ���ɹ�
	{
		LOG_W("mo_netconn_create", "netconn create faild.");						// �½�ʵ������ ���������Ϣ
	}

    ip_addr_t server_addr = {0};													// �½�һ�� IP ��ַ�Ļ�����
    inet_aton(REMOTE_IP, &server_addr);												// ��Զ�� IP ��ַת��Ϊ������ʽ
	molink_result = mo_netconn_connect(molink_modle, test_netconn, server_addr, REMOTE_PORT);	// �Ըõ�ַ��������
	if(molink_result != OS_EOK)														// �ж��Ƿ����ӽ����ɹ�
	{
		LOG_W("mo_netconn_connect", "netconn connect faild.");						// �������Ӵ��� ���������Ϣ
	}
	else
	{
		LOG_I("mo_netconn_connect", "remote IP:%s.", REMOTE_IP);					// �������ӳɹ� ���Զ�� IP
	}

    os_size_t sent_size = mo_netconn_send(molink_modle, test_netconn, "Hello", 5);	// ��Զ�˷�������
	if(sent_size != 5)																// �жϷ������ݳ��ȸ�
	{
		LOG_W("mo_netconn_send", "netconn send faild.");
	}
	else
	{
		LOG_I("mo_netconn_send", "send success.");
	}

    void *data_ptr = OS_NULL;
    os_size_t data_size = 0;
    while (1)
    {
		molink_result = mo_netconn_recv(molink_modle, test_netconn, &data_ptr, &data_size, OS_WAIT_FOREVER);
		if(molink_result != OS_EOK)
		{
			LOG_W("mo_netconn_recv", "netconn recv faild.");
		}
		else
		{
			LOG_I("mo_netconn_recv",
				"Module %s, netconn %d, receive data_len: %d, message: %s.",
				molink_modle->name,
				test_netconn->connect_id,
				data_size,
				data_ptr
			);
			sent_size = mo_netconn_send(molink_modle, test_netconn, data_ptr, data_size);
			if(sent_size != data_size)
			{
				LOG_W("mo_netconn_send", "netconn send faild.");
			}
			else
			{
				LOG_I("mo_netconn_send", "send success.");
			}
		}
    }
}

int main(void)
{
    os_task_t *task;

    task = os_task_create("user", user_task, NULL, 1024*8, 3);
    OS_ASSERT(task);
    os_task_startup(task);

    return 0;
}
