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

// *************************** 例程说明 ***************************
// 
// 测试需要准备 <OneOS 启物开发板> 一块
// 推荐 MDK 版本为 MDK5.28a
// 
// 调试下载需要准备 <逐飞科技 CMSIS-DAP 调试下载器> 或 <ARM 调试下载器> 一个
// 或者使用 <标准 Jlink> 进行下载调试连接亦可
// 
// 确认连接无误 上电 可使用调试下载器供电 但推荐使用 USB供电 可通过CH340的Type-C进行供电
// 烧录本例程后 如果使用的是 <逐飞科技 CMSIS-DAP 调试下载器> 或 <ARM 调试下载器>
// 可以直接在串口助手中打开对应串口
// 如果使用的并不是 <逐飞科技 CMSIS-DAP 调试下载器> 或 <ARM 调试下载器>
// 请接上 CH340 标识的Type-C接口到电脑USB 再打开串口调试助手打开对应串口
// 
// 本例程示范将 开发板上ESP8266WIFI模块通过Molink组件初始化
// 输出自身IP地址 进行Ping测试 
// 然后构建 TCP 连接 并且对远端地址进行收发数据测试
// 相关信息通过 shell 输出
// 
// 需要先行设置 
// OneOS-Cube->Components->Network->Molink->Enable IoT Modules->WiFi Modules Support->ESP8266->ESP8266 Config
// ESP8266 Connect AP SSID 为测试环境的 WiFi 名称/SSID
// ESP8266 Connect AP Password 为测试环境的 WiFi 密码/Password
// 
// 本例程需要一个可以使用网络调试功能的调试助手
// 可以使用 sscom 端口设置为 TCP_Server 并更根据地址更改下方 REMOTE_IP
// 端口默认设置为 8080 端口
// 设置完成后点击侦听准备好测试
// 
// 确认准备完成后烧录程序开始运行测试
// 
// 打开新的工程或者工程移动了位置务必执行以下操作
// 第一步 关闭上面所有打开的文件
// 第二步 project->clean  等待下方进度条走完
// 
// 2021-08-12	SEEKFREE
// 
// *************************** 例程说明 ***************************

#define PING_IP				"192.168.2.1"											// PING 测试的对象地址 这里可以设置为网关 根据测试自行修改
#define REMOTE_IP			"192.168.2.16"											// 远端通信地址 这里设置为测试时使用的的电脑的IP
#define REMOTE_PORT			8080													// 通信端口

static void user_task(void *parameter)
{
	os_err_t molink_result = OS_ERROR;												// 执行状态变量

	mo_object_t *molink_modle = mo_get_by_name(ESP8266_NAME);						// 获取 ESP8266 对象
	OS_ASSERT(molink_modle != OS_NULL);												// 确认对象实例正常

	char molink_data_buffer[16];													// 新建一个缓冲区用来存放 IP 地址
	molink_result = mo_get_ipaddr(molink_modle, molink_data_buffer);				// 获取自身的 IP 地址
	if(molink_result != OS_EOK)														// 判断获取是否成功
	{
		LOG_W("mo_get_ipaddr", "get ipaddr faild.");								// 获取地址失败 输出错误信息
	}
	else
	{
		LOG_I("mo_get_ipaddr", "IP:%s.", molink_data_buffer);						// 获取地址成功 输出 IP 地址信息
	}

	ping_resp_t ping_test_t;														// 新建 ping 测试结构体
	molink_result = mo_ping(molink_modle, PING_IP, 64, 10, &ping_test_t);			// ping 测试
	if(molink_result != OS_EOK)														// 判断是否测试成功
	{
		LOG_W("mo_ping", "ping faild.");											// ping 测试失败 输出错误信息
	}
	else
	{
		LOG_I("mo_ping", "PING IP:%s | LEN:%d | TIME OUT:%d | TIME RESPONSE:%d.", 	// ping 测试成功 输出 ping 结果信息
			inet_ntoa(ping_test_t.ip_addr.addr),									// 将 IP 从数字模式转换为字符串
			ping_test_t.data_len,													// 显示 ping 测试的数据长度
			ping_test_t.ttl,														// 显示超时时长
			ping_test_t.time);														// 显示延迟时间
	}

	mo_netconn_t *test_netconn = mo_netconn_create(molink_modle, NETCONN_TYPE_TCP);	// 新建构建 netceonn 的对象实例 方式为 TCP
	if(test_netconn == OS_NULL)														// 判断是否新建实例成功
	{
		LOG_W("mo_netconn_create", "netconn create faild.");						// 新建实例出错 输出错误信息
	}

    ip_addr_t server_addr = {0};													// 新建一个 IP 地址的缓冲区
    inet_aton(REMOTE_IP, &server_addr);												// 将远端 IP 地址转换为数字形式
	molink_result = mo_netconn_connect(molink_modle, test_netconn, server_addr, REMOTE_PORT);	// 对该地址建立连接
	if(molink_result != OS_EOK)														// 判断是否连接建立成功
	{
		LOG_W("mo_netconn_connect", "netconn connect faild.");						// 建立连接错误 输出错误信息
	}
	else
	{
		LOG_I("mo_netconn_connect", "remote IP:%s.", REMOTE_IP);					// 建立连接成功 输出远端 IP
	}

    os_size_t sent_size = mo_netconn_send(molink_modle, test_netconn, "Hello", 5);	// 对远端发送数据
	if(sent_size != 5)																// 判断发送数据长度跟
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
