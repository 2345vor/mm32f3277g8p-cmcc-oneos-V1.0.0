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
// 按下主板上对应按键后 程序会通过 shell 输出信息
// 
// 本例程示范将 S1初始化为普通IO按键 S2/S3初始化为中断输入按键
// 按下S1会输出按键计数复位信息
// 按下S2会输出按键计数增加信息
// 按下S3会输出按键计数减少信息
// 
// 打开新的工程或者工程移动了位置务必执行以下操作
// 第一步 关闭上面所有打开的文件
// 第二步 project->clean  等待下方进度条走完
// 
// 2021-08-12	SEEKFREE
// 
// *************************** 例程说明 ***************************

void key2_handler (void *args)																// 按键 S2 的回调处理函数
{
	*((os_uint32_t *)args) += 1;															// 按键 S3 触发中断回调 将传参的指针指向的 key_count 加一
}

void key3_handler (void *args)																// 按键 S3 的回调处理函数
{
	*((os_uint32_t *)args) -= 1;															// 按键 S3 触发中断回调 将传参的指针指向的 key_count 减一
}

static void user_task(void *parameter)
{
    os_uint32_t key_count = 0;
    os_uint32_t key_count_last = 0;
	os_uint8_t key_state = 0;

	os_pin_mode(key_table[0].pin, key_table[0].mode);										// 将按键初始化为 普通IO 输入模式 参数详见 bsp/board.c
	os_pin_attach_irq(key_table[1].pin, key_table[1].irq_mode, key2_handler, &key_count);	// 将按键初始化为 中断输入IO 下降沿触发 参数详见 bsp/board.c 关联回调函数 key2_handler 传参 key_count 的地址
	os_pin_attach_irq(key_table[2].pin, key_table[2].irq_mode, key3_handler, &key_count);	// 将按键初始化为 中断输入IO 下降沿触发 参数详见 bsp/board.c 关联回调函数 key3_handler 传参 key_count 的地址
	os_pin_irq_enable(key_table[1].pin, ENABLE);											// 使能按键的中断
	os_pin_irq_enable(key_table[2].pin, ENABLE);											// 使能按键的中断
	key_state = os_pin_read(key_table[0].pin);												// 获取默认的 S1 按键状态

	os_task_msleep(1000);
	os_kprintf("\r\nkey test is running.\r\n");
    while (1)
    {
		if(os_pin_read(key_table[0].pin) != key_state)										// 按键 S1 状态发生变化
		{
			key_count = 0;																	// 清空 key_count 计数
			os_kprintf("key_count was reset.\r\n");											// 通过 shell 输出计数复位提示信息
			os_task_msleep(500);															// 延时 主要是防止反复触发消抖
		}
		key_state = os_pin_read(key_table[0].pin);											// 更新按键 S1 状态

		if(key_count_last != key_count)														// 计数状态变更
		{
			key_count_last = key_count;														// 更新计数状态
			os_kprintf("key_count is %d.\r\n", key_count);									// 输出当前按键计数状态
		}
		os_task_msleep(10);
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
