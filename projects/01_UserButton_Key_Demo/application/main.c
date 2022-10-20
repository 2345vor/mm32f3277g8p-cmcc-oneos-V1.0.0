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
// ���������϶�Ӧ������ �����ͨ�� shell �����Ϣ
// 
// ������ʾ���� S1��ʼ��Ϊ��ͨIO���� S2/S3��ʼ��Ϊ�ж����밴��
// ����S1���������������λ��Ϣ
// ����S2�������������������Ϣ
// ����S3�������������������Ϣ
// 
// ���µĹ��̻��߹����ƶ���λ�����ִ�����²���
// ��һ�� �ر��������д򿪵��ļ�
// �ڶ��� project->clean  �ȴ��·�����������
// 
// 2021-08-12	SEEKFREE
// 
// *************************** ����˵�� ***************************

void key2_handler (void *args)																// ���� S2 �Ļص�������
{
	*((os_uint32_t *)args) += 1;															// ���� S3 �����жϻص� �����ε�ָ��ָ��� key_count ��һ
}

void key3_handler (void *args)																// ���� S3 �Ļص�������
{
	*((os_uint32_t *)args) -= 1;															// ���� S3 �����жϻص� �����ε�ָ��ָ��� key_count ��һ
}

static void user_task(void *parameter)
{
    os_uint32_t key_count = 0;
    os_uint32_t key_count_last = 0;
	os_uint8_t key_state = 0;

	os_pin_mode(key_table[0].pin, key_table[0].mode);										// ��������ʼ��Ϊ ��ͨIO ����ģʽ ������� bsp/board.c
	os_pin_attach_irq(key_table[1].pin, key_table[1].irq_mode, key2_handler, &key_count);	// ��������ʼ��Ϊ �ж�����IO �½��ش��� ������� bsp/board.c �����ص����� key2_handler ���� key_count �ĵ�ַ
	os_pin_attach_irq(key_table[2].pin, key_table[2].irq_mode, key3_handler, &key_count);	// ��������ʼ��Ϊ �ж�����IO �½��ش��� ������� bsp/board.c �����ص����� key3_handler ���� key_count �ĵ�ַ
	os_pin_irq_enable(key_table[1].pin, ENABLE);											// ʹ�ܰ������ж�
	os_pin_irq_enable(key_table[2].pin, ENABLE);											// ʹ�ܰ������ж�
	key_state = os_pin_read(key_table[0].pin);												// ��ȡĬ�ϵ� S1 ����״̬

	os_task_msleep(1000);
	os_kprintf("\r\nkey test is running.\r\n");
    while (1)
    {
		if(os_pin_read(key_table[0].pin) != key_state)										// ���� S1 ״̬�����仯
		{
			key_count = 0;																	// ��� key_count ����
			os_kprintf("key_count was reset.\r\n");											// ͨ�� shell ���������λ��ʾ��Ϣ
			os_task_msleep(500);															// ��ʱ ��Ҫ�Ƿ�ֹ������������
		}
		key_state = os_pin_read(key_table[0].pin);											// ���°��� S1 ״̬

		if(key_count_last != key_count)														// ����״̬���
		{
			key_count_last = key_count;														// ���¼���״̬
			os_kprintf("key_count is %d.\r\n", key_count);									// �����ǰ��������״̬
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
