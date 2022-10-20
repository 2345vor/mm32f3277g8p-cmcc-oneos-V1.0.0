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
#include <sensors/sensor.h>

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
// 本例程示范将 开发板上AHT20传感器通过软件IIC1总线初始化
// 每间隔一秒输出一次当前温湿度信息
// 相关信息通过 shell 输出
// 
// 打开新的工程或者工程移动了位置务必执行以下操作
// 第一步 关闭上面所有打开的文件
// 第二步 project->clean  等待下方进度条走完
// 
// 2021-08-12	SEEKFREE
// 
// *************************** 例程说明 ***************************

static void user_task(void *parameter)
{
    struct os_sensor_data sensor_data;																				// 新建一个数据存放结构体

	os_task_msleep(1000);
	os_kprintf("\r\nAHT20 test running.\r\n");

	// 为什么 AHT20 需要两个设备 这是因为驱动文件中将其拆分成了两个部分 分别是湿度与温度 注册成了两个设备
    os_device_t *sensor_humi = os_device_find("humi_aht20");														// 通过设备名找到 humi_aht20
    OS_ASSERT(sensor_humi != NULL);																					// 如果没有该设备则报错
    os_device_open(sensor_humi);																					// 找到设备就打开设备
	
    os_device_t *sensor_temp = os_device_find("temp_aht20");														// 通过设备名找到 temp_aht20
    OS_ASSERT(sensor_temp != NULL);																					// 如果没有该设备则报错
    os_device_open(sensor_temp);																					// 找到设备就打开设备

	while(1)
	{
        os_device_read_nonblock(sensor_humi, 0, &sensor_data, sizeof(struct os_sensor_data));						// 通过设备读取数据
        os_kprintf("sensor humi (%d.%03d)%\r\n", sensor_data.data.humi / 1000, sensor_data.data.humi % 1000);		// 将数据通过 shell 输出

        os_device_read_nonblock(sensor_temp, 0, &sensor_data, sizeof(struct os_sensor_data));						// 通过设备读取数据
        os_kprintf("sensor temp (%d.%03d)℃\r\n", sensor_data.data.temp / 1000, sensor_data.data.temp % 1000);		// 将数据通过 shell 输出

		os_task_msleep(1000);
	}
}

int main(void)
{
    os_task_t *task;

    task = os_task_create("user", user_task, NULL, 1024, 3);
    OS_ASSERT(task);
    os_task_startup(task);

    return 0;
}
