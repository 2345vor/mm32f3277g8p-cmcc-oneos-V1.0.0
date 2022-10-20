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
// 本例程示范将 B1初始化为IO控制蜂鸣器
// 跟随时间蜂鸣器间隔会变短
// 最终长鸣后重新开始循环
// 会有信息通过 shell 输出
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
	os_uint16_t count = 950;

	os_task_msleep(1000);
    os_pin_mode(beep_table[0].pin, PIN_MODE_OUTPUT);

    while (1)
    {
		os_pin_write(beep_table[0].pin, beep_table[0].active_level);
		os_task_msleep(50);
		os_pin_write(beep_table[0].pin, !beep_table[0].active_level);
		os_task_msleep(count);
		os_kprintf("count: %d\r\n",count);

		count -= 10;
		if(count == 0)
		{
			os_kprintf("\r\nBOOM!");
			os_pin_write(beep_table[0].pin, beep_table[0].active_level);
			os_task_msleep(1000);
			os_pin_write(beep_table[0].pin, !beep_table[0].active_level);
			os_task_msleep(1000);
			count = 950;
		}
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
