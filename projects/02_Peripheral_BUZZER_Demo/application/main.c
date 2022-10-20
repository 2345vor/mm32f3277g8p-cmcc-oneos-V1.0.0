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
// ������ʾ���� B1��ʼ��ΪIO���Ʒ�����
// ����ʱ��������������
// ���ճ��������¿�ʼѭ��
// ������Ϣͨ�� shell ���
// 
// ���µĹ��̻��߹����ƶ���λ�����ִ�����²���
// ��һ�� �ر��������д򿪵��ļ�
// �ڶ��� project->clean  �ȴ��·�����������
// 
// 2021-08-12	SEEKFREE
// 
// *************************** ����˵�� ***************************

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
