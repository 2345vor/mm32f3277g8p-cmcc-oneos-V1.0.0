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
 * @file        lpmgr.c
 *
 * @brief       this file implements Low power management related functions
 *
 * @details
 *
 * @revision
 * Date          Author          Notes
 * 2020-02-20    OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <string.h>
#include <stdlib.h>
#include <arch_interrupt.h>
#include <os_clock.h>
#include <os_timer.h>
#include <os_memory.h>
#include <os_assert.h>
#include <os_errno.h>
#include <lpmgr.h>
#include <dlog.h>

#ifdef OS_USING_TICKLESS_LPMGR

#define DBG_TAG "lpmgr"

static struct os_lpmgr_dev *gs_lpmgr = NULL;
static struct lpmgr_notify gs_lpmgr_notify;

#define LPMGR_TICKLESS_THRESH (2)


struct os_lpmgr_dev *os_lpmgr_dev_get(void)
{
    return gs_lpmgr;
}

OS_WEAK os_uint32_t lpmgr_enter_critical(os_uint8_t sleep_mode)
{
    return os_irq_lock();
}

OS_WEAK void lpmgr_exit_critical(os_uint32_t ctx, os_uint8_t sleep_mode)
{
    os_irq_unlock(ctx);
}

static int lpmgr_device_suspend(lpmgr_sleep_mode_e mode)
{
    int index, ret = OS_EOK;
    struct os_lpmgr_dev *lpm;

    lpm = os_lpmgr_dev_get();

    for (index = 0; index < lpm->device_number; index++)
    {
        if (lpm->lp_device[index].ops->suspend != OS_NULL)
        {
            ret = lpm->lp_device[index].ops->suspend(lpm->lp_device[index].device, mode);
            if (ret != OS_EOK)
                break;
        }
    }

    return ret;
}

static void lpmgr_device_resume(lpmgr_sleep_mode_e mode)
{
    int index;
    struct os_lpmgr_dev *lpm;

    lpm = os_lpmgr_dev_get();

    for (index = 0; index < lpm->device_number; index++)
    {
        if (lpm->lp_device[index].ops->resume != OS_NULL)
        {
            lpm->lp_device[index].ops->resume(lpm->lp_device[index].device, mode);
        }
    }
}


os_tick_t timer_next_timeout_tick(void)
{
    return os_tickless_get_sleep_ticks();
}

int lpmgr_enter_sleep_call(lpmgr_sleep_mode_e mode)
{
    int ret = OS_EOK;
    /* Notify app will enter sleep mode */
    if (gs_lpmgr_notify.notify)
        gs_lpmgr_notify.notify(SYS_ENTER_SLEEP, mode, gs_lpmgr_notify.data);

    /* Suspend all peripheral device */
    ret = lpmgr_device_suspend(mode);
    if (ret != OS_EOK)
    {
        lpmgr_device_resume(mode);
        if (gs_lpmgr_notify.notify)
            gs_lpmgr_notify.notify(SYS_EXIT_SLEEP, mode, gs_lpmgr_notify.data);

        return OS_ERROR;
    }

    return OS_EOK;
}

int lpmgr_exit_sleep_call(lpmgr_sleep_mode_e mode)
{
    /* resume all device */
    lpmgr_device_resume(mode);

    if (gs_lpmgr_notify.notify)
        gs_lpmgr_notify.notify(SYS_EXIT_SLEEP, mode, gs_lpmgr_notify.data);

    return OS_EOK;
}

OS_WEAK os_err_t other_check(struct os_lpmgr_dev *lpm)
{
    return OS_EOK;
}


os_err_t is_enter_lpmgr(struct os_lpmgr_dev *lpm)
{
    
    if (lpm == NULL)
    {
        return OS_EEMPTY;
    }

    if (!lpm->init_falg)
    {
        return OS_EFULL;
    }

    if (!(lpm->sleep_mask & (1 << lpm->sleep_mode)))
    {
        return OS_EINVAL;
    }

    if (OS_EOK != other_check(lpm))
    {
        return OS_EBUSY;
    }
    
    return OS_EOK;
}

void lpmgr_updatetick(os_tick_t delt_tick, os_tick_t timeout_tick)
{
    os_tick_t update_tick;
    
    if (delt_tick < timeout_tick)
    {
        update_tick = delt_tick;
    }
    else
    {
        update_tick = timeout_tick;
    }

    os_tickless_update(update_tick);
}


os_err_t lpmgr_start(struct os_lpmgr_dev *lpm)
{
    os_err_t ret = OS_EOK;
    os_tick_t delt_tick;
    os_tick_t timeout_tick = 0;
    const struct os_lpmgr_ops *lpm_ops = lpm->ops;
    
    ret = lpmgr_enter_sleep_call(lpm->sleep_mode);
    if (ret != OS_EOK)
    {
        return ret;
    }

    timeout_tick = timer_next_timeout_tick();
    if (timeout_tick < lpm->min_tick)
    {
        lpmgr_exit_sleep_call(lpm->sleep_mode);
        return OS_EBUSY;
    }
    
    if (timeout_tick > lpm->max_tick)
    {
        timeout_tick = lpm->max_tick;
    }

    lpm_ops->sleeptick_start(lpm, timeout_tick);

    lpm_ops->sleep(lpm->sleep_mode);

    /* get lptim tick */
    delt_tick = lpm_ops->sleeptick_gettick();
    lpm_ops->sleeptick_stop();

    /* update lptim to systick */
    lpmgr_updatetick(delt_tick, timeout_tick);
    LOG_I(DBG_TAG, "[%s]-[%d], timeout_tick[%d], delt_tick[%d], cur_tick[%d]\r\n", __FILE__, __LINE__, timeout_tick, delt_tick, os_tick_get());
    lpmgr_exit_sleep_call(lpm->sleep_mode);
    
    return OS_EOK;
}


void os_low_power_manager(void)
{
    os_err_t ret;
    struct os_lpmgr_dev *lpm;
    os_uint32_t level;

    lpm = os_lpmgr_dev_get();
    if (lpm == NULL)
    {
        return;
    }
    
    level = lpmgr_enter_critical(lpm->sleep_mode);
    ret = is_enter_lpmgr(lpm);
    if (ret == OS_EOK)
    {
        lpmgr_start(lpm);
    }
    lpmgr_exit_critical(level, lpm->sleep_mode);
}


static void lpmgr_check_sleep_mode(struct os_lpmgr_dev *lpm)
{
    lpmgr_sleep_mode_e index;
    lpmgr_sleep_mode_e sleepmode = lpm->sleep_mode;

    for (index = SYS_SLEEP_MODE_NONE; index < SYS_SLEEP_MODE_MAX; index++)
    {
        if (lpm->modes[index])
        {
            sleepmode = index;
            break;
        }
    }
    
    if (lpm->sleep_mode != sleepmode)
    {
        lpm->sleep_mode = sleepmode;
    }
}

/**
 ***********************************************************************************************************************
 * @brief           Upper application or device driver requests the system stall in corresponding power mode
 *
 * @param[in]       sleep_mode                the parameter of run mode or sleep mode
 *
 * @return          no return value
 ***********************************************************************************************************************
 */
void os_lpmgr_request(lpmgr_sleep_mode_e sleep_mode)
{
    os_base_t     level;
    struct os_lpmgr_dev *lpm;
    lpm = os_lpmgr_dev_get();

    if (lpm->init_falg != 1)
        return;

    if (sleep_mode > (SYS_SLEEP_MODE_MAX - 1))
        return;

    level = os_irq_lock();
    if (lpm->modes[sleep_mode] < 255)
        lpm->modes[sleep_mode]++;

    lpmgr_check_sleep_mode(lpm);
	
    os_irq_unlock(level);
}

/**
 ***********************************************************************************************************************
 * @brief           Upper application or device driver releases the system stall in corresponding power mode
 *
 * @param[in]       sleep_mode                the parameter of run mode or sleep mode
 *
 * @return          no return value
 ***********************************************************************************************************************
 */
void os_lpmgr_release(lpmgr_sleep_mode_e sleep_mode)
{
    os_ubase_t    level;
    struct os_lpmgr_dev *lpm;

    lpm = os_lpmgr_dev_get();

    if (lpm->init_falg != 1)
        return;

    if (sleep_mode > (SYS_SLEEP_MODE_MAX - 1))
        return;

    level = os_irq_lock();
    if (lpm->modes[sleep_mode] > 0)
        lpm->modes[sleep_mode]--;

    lpmgr_check_sleep_mode(lpm);
	
    os_irq_unlock(level);
}



/**
 ***********************************************************************************************************************
 * @brief           Register a device with PM feature
 *
 * @param[in]       device              pointer of os device with lpmgr feature
 * @param[in]       ops                 pointer of lpmgr device operation function set
 *
 * @return          no return value
 ***********************************************************************************************************************
 */
os_err_t os_lpmgr_device_register(struct os_device *device, const struct os_lpmgr_device_ops *ops)
{
    struct os_lpmgr_device *device_lpm;
    os_uint32_t index;
    struct os_lpmgr_dev *lpm;

    lpm = os_lpmgr_dev_get();
    OS_ASSERT(lpm != NULL);

    for (index = 0; index < lpm->device_number; index++)
    {
        if ((lpm->lp_device[index].device == device) && (lpm->lp_device[index].ops == ops))
        {
            LOG_D(DBG_TAG, "dev[%s], ops[%p] alread register!\n", device->name, ops);
            return OS_EOK;
        }
    }

    device_lpm = (struct os_lpmgr_device *)os_realloc(lpm->lp_device,
                                                      (lpm->device_number + 1) * sizeof(struct os_lpmgr_device));
    if (device_lpm != OS_NULL)
    {
        lpm->lp_device                                = device_lpm;
        lpm->lp_device[lpm->device_number].device = device;
        lpm->lp_device[lpm->device_number].ops    = ops;
        lpm->device_number += 1;
    }

    LOG_D(DBG_TAG, "register dev[%s], ops[%p]!\n", device->name, ops);

    return OS_EOK;
}

/**
 ***********************************************************************************************************************
 * @brief           Unregister device from low power manager
 *
 * @param[in]       device              pointer of device with lpmgr feature
 *
 * @return          no return value
 ***********************************************************************************************************************
 */
void os_lpmgr_device_unregister(struct os_device *device, const struct os_lpmgr_device_ops *ops)
{
    os_uint32_t index;
    os_uint8_t del_flag = 0;
    struct os_lpmgr_dev *lpm;

    lpm = os_lpmgr_dev_get();
    OS_ASSERT(lpm != NULL);

    for (index = 0; index < lpm->device_number; index++)
    {
        if ((lpm->lp_device[index].device == device) && (lpm->lp_device[index].ops == ops))
        {
            /* remove current entry */
            for (; index < lpm->device_number - 1; index++)
            {
                lpm->lp_device[index] = lpm->lp_device[index + 1]; /* copy and move */
            }

            lpm->lp_device[lpm->device_number - 1].device = OS_NULL;
            lpm->lp_device[lpm->device_number - 1].ops    = OS_NULL;

            lpm->device_number -= 1;
            del_flag = 1;
            /* break out and not touch memory */
            break;
        }
    }

    if (0 == del_flag)
    {
        LOG_D(DBG_TAG, "not found dev[%s], ops[%p]!\n", device->name, ops);
    }
    
}

/**
 ***********************************************************************************************************************
 * @brief           set notification callback for application
 *
 * @param[in]       notify              pointer of notify
 * @param[in]       data                pointer of data
 *
 * @return          no return value
 ***********************************************************************************************************************
 */
void os_lpmgr_notify_set(void (*notify)(os_lpmgr_sys_e event, lpmgr_sleep_mode_e mode, void *data), void *data)
{
    gs_lpmgr_notify.notify = notify;
    gs_lpmgr_notify.data   = data;

    LOG_D(DBG_TAG, "register lpmgr_notify notify[%p]!\n", notify);
}


os_err_t  lpmgr_device_init(os_device_t *dev)
{
    struct os_lpmgr_dev *lpm;

    lpm = (struct os_lpmgr_dev *)dev;
    OS_ASSERT(lpm != OS_NULL);

    lpm->init_falg = 1;

    return OS_EOK;
}


const static struct os_device_ops lpm_ops = {
    .init    = lpmgr_device_init,
};

/**
 ***********************************************************************************************************************
 * @brief           initialize low power manager
 *
 * @param[in]       ops             pointer of lpmgr operation function set
 * @param[in]       timer_mask      indicates which mode has timer feature
 * @param[in]       user_data       user_data
 *
 * @return          no return value
 ***********************************************************************************************************************
 */
void os_lpmgr_register(struct os_lpmgr_dev  *lpmgr, os_uint8_t sleep_mask, void *user_data)
{
    struct os_device *device;
    struct os_lpmgr_dev *lpm;

    lpm    = lpmgr;
    device = &(lpmgr->parent);

    device->type        = OS_DEVICE_TYPE_PM;

    device->ops = &lpm_ops;
    device->user_data = user_data;

    /* register low power manager device to the system */
    os_device_register(device, OS_LPMGR_DEVICE_NAME);

    lpm->sleep_mode = SYS_SLEEP_MODE_IDLE;
    lpm->sleep_mask = sleep_mask;

    lpm->lp_device     = OS_NULL;
    lpm->device_number = 0;

}

os_device_t *os_lpmgr_set_device(const char *name)
{
    os_device_t *new;
    
    /* Find new console device */
    new = os_device_find(name);
    if (OS_NULL != new)
    {
        /* Set new console device */
        os_device_open(new);

        gs_lpmgr = (struct os_lpmgr_dev *)new;
    }

    return new;
}

os_err_t os_lpmgr_dev_init(void)
{
    os_lpmgr_set_device(OS_LPMGR_DEVICE_NAME);
    
    return OS_EOK;
}

OS_POSTCORE_INIT(os_lpmgr_dev_init, OS_INIT_SUBLEVEL_LOW);


#ifdef OS_USING_SHELL
#include <shell.h>

static const char *gs_lpmgr_sleep_str[] = SYS_SLEEP_MODE_NAMES;

static void lpmgr_release_mode(int argc, char **argv)
{
    lpmgr_sleep_mode_e mode = SYS_SLEEP_MODE_NONE;
    if (argc >= 2)
    {
        mode = (lpmgr_sleep_mode_e)atoi(argv[1]);
    }

    os_lpmgr_release(mode);
}
SH_CMD_EXPORT(power_release, lpmgr_release_mode, "release power management mode");

static void lpmgr_request_mode(int argc, char **argv)
{
    lpmgr_sleep_mode_e mode = SYS_SLEEP_MODE_NONE;
    if (argc >= 2)
    {
        mode = (lpmgr_sleep_mode_e)atoi(argv[1]);
    }

    os_lpmgr_request(mode);
}
SH_CMD_EXPORT(power_request, lpmgr_request_mode, "request power management mode");


static void lpmgr_dump_status(void)
{
    os_uint32_t   index;
    struct os_lpmgr_dev *lpm;

    lpm = gs_lpmgr;

    os_kprintf("| Power Management Mode | Counter | Timer |\n");
    os_kprintf("+-----------------------+---------+-------+\n");
    for (index = 0; index < SYS_SLEEP_MODE_MAX; index++)
    {
        int has_timer = 0;
        if (lpm->sleep_mask & (1 << index))
            has_timer = 1;

        os_kprintf("| %021s | %7d | %5d |\n", gs_lpmgr_sleep_str[index], lpm->modes[index], has_timer);
    }
    os_kprintf("+-----------------------+---------+-------+\n");

    os_kprintf("lpmgr current sleep mode: %s\n", gs_lpmgr_sleep_str[lpm->sleep_mode]);
}
SH_CMD_EXPORT(power_status, lpmgr_dump_status, "dump power management status");

static void lpmgr_dump_dev(void)
{
    os_uint32_t index;
    struct os_lpmgr_dev *lpm;
    lpm = os_lpmgr_dev_get();

    os_kprintf("| %-5s | %-20s | %-10s |\n", "no", "dev name", "ops");
    os_kprintf("+-------+----------------------+------------+\n");

    for (index = 0; index < lpm->device_number; index++)
    {
        os_kprintf("| %-5d | %-20s | 0x%p |\n", index, lpm->lp_device[index].device->name, lpm->lp_device[index].ops);
    }
    os_kprintf("total register num: %d\n", lpm->device_number);

}
SH_CMD_EXPORT(power_dev, lpmgr_dump_dev, "dump power management dev");


#endif


#endif /* OS_USING_TICKLESS_LPMGR */

