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
 * @file        wdg_test.c
 *
 * @brief       The test file for wdg.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <os_task.h>
#include <device.h>
#include <os_errno.h>
#include <watchdog/watchdog.h>
#include <string.h>
#include <shell.h>

#define WDT_DEVICE_NAME "iwdg1"

static int wdg_test(int argc, char *argv[])
{
    os_err_t    ret     = OS_EOK;
    os_uint32_t timeout = 10;
    char        device_name[OS_NAME_MAX];
    int         count = 0;
    os_device_t *wdg_dev;
    
    if (argc == 2)
    {
        strncpy(device_name, argv[1], OS_NAME_MAX);
    }
    else
    {
        strncpy(device_name, WDT_DEVICE_NAME, OS_NAME_MAX);
    }

    wdg_dev = os_device_find(device_name);
    if (!wdg_dev)
    {
        os_kprintf("find %s failed!\r\n", device_name);
        return OS_ERROR;
    }

    ret = os_device_open(wdg_dev);
    if (ret != OS_EOK)
    {
        os_kprintf("initialize %s failed!\r\n", device_name);
        return OS_ERROR;
    }

    ret = os_device_control(wdg_dev, OS_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != OS_EOK)
    {
        os_kprintf("%s not support set timeout!\r\n", device_name);
    }

    ret = os_device_control(wdg_dev, OS_DEVICE_CTRL_WDT_START, OS_NULL);
    if (ret != OS_EOK)
    {
        os_kprintf("start %s failed!\r\n", device_name);
        return OS_ERROR;
    }

    os_device_control(wdg_dev, OS_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
    while (count < 11)
    {
        os_task_msleep(1000);
        count++;
        os_kprintf("watch dog keep alive for :%ds\r\n", count);
        os_device_control(wdg_dev, OS_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
    }
    count = 0;
    os_kprintf("\r\n");
    while (1)
    {
        os_kprintf("watch dog stop feed for :%d.%ds\r\n", count / 10, count % 10);
        os_task_msleep(100);
        count++;
    }
}

SH_CMD_EXPORT(wdg_test, wdg_test, "test Independent watchdog!");
