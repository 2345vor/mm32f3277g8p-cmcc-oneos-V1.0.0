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
// ������ʾ���� ��������AHT20������ͨ�����IIC1���߳�ʼ��
// ÿ���һ�����һ�ε�ǰ��ʪ����Ϣ
// �����Ϣͨ�� shell ���
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
    struct os_sensor_data sensor_data;																				// �½�һ�����ݴ�Žṹ��

	os_task_msleep(1000);
	os_kprintf("\r\nAHT20 test running.\r\n");

	// Ϊʲô AHT20 ��Ҫ�����豸 ������Ϊ�����ļ��н����ֳ����������� �ֱ���ʪ�����¶� ע����������豸
    os_device_t *sensor_humi = os_device_find("humi_aht20");														// ͨ���豸���ҵ� humi_aht20
    OS_ASSERT(sensor_humi != NULL);																					// ���û�и��豸�򱨴�
    os_device_open(sensor_humi);																					// �ҵ��豸�ʹ��豸
	
    os_device_t *sensor_temp = os_device_find("temp_aht20");														// ͨ���豸���ҵ� temp_aht20
    OS_ASSERT(sensor_temp != NULL);																					// ���û�и��豸�򱨴�
    os_device_open(sensor_temp);																					// �ҵ��豸�ʹ��豸

	while(1)
	{
        os_device_read_nonblock(sensor_humi, 0, &sensor_data, sizeof(struct os_sensor_data));						// ͨ���豸��ȡ����
        os_kprintf("sensor humi (%d.%03d)%\r\n", sensor_data.data.humi / 1000, sensor_data.data.humi % 1000);		// ������ͨ�� shell ���

        os_device_read_nonblock(sensor_temp, 0, &sensor_data, sizeof(struct os_sensor_data));						// ͨ���豸��ȡ����
        os_kprintf("sensor temp (%d.%03d)��\r\n", sensor_data.data.temp / 1000, sensor_data.data.temp % 1000);		// ������ͨ�� shell ���

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
