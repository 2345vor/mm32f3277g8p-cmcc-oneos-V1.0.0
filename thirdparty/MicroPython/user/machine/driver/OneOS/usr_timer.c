#include <string.h>
#include "usr_misc.h"
#include "py/runtime.h"

#include "os_task.h"
#include "os_clock.h"
#include "os_timer.h"
#include "usr_timer.h"
#include <os_memory.h>

mp_uint_t mp_hal_ticks_us(void) {
	return os_tick_get() * 1000000UL / OS_TICK_PER_SECOND;
}

mp_uint_t mp_hal_ticks_ms(void) {
	return os_tick_get() * 1000 / OS_TICK_PER_SECOND;
}

mp_uint_t mp_hal_ticks_cpu(void) {
	return os_tick_get();
}

void mp_hal_delay_us(mp_uint_t us) {

	os_tick_t t0 = os_tick_get(), t1, dt;
    uint64_t dtick = us * OS_TICK_PER_SECOND / 1000000L;
    while (1) {
        t1 = os_tick_get();
        dt = t1 - t0;
        if (dt >= dtick) {
            break;
        }
        mp_handle_pending();
    }
}


void mp_hal_delay_ms(mp_uint_t ms) {

	os_tick_t t0 = os_tick_get(), t1, dt;
    uint64_t dtick = ms * OS_TICK_PER_SECOND / 1000L;
    while (1) {
        t1 = os_tick_get();
        dt = t1 - t0;
        if (dt >= dtick) {
            break;
        }
        MICROPY_EVENT_POLL_HOOK;
        os_task_tsleep(1);
    }
}

#if (MICROPY_PY_MACHINE_TIMER) 


static int timer_enable(void *device, uint16_t flag)
{
    device_info_t *timer =  ((machine_timer_obj_t *)device)->timer;
    return os_timer_start((os_timer_t *)(timer->other));
}

static int timer_disable(void *device)
{
    device_info_t *timer =  ((machine_timer_obj_t *)device)->timer;
    os_timer_stop((os_timer_t *)(timer->other));
    return MP_MACHINE_OP_EOK;
}


static uint8_t timer_get_os_timer_flag(uint8_t mpy_flag)
{
    uint8_t flag = 0;

    if (mpy_flag == MP_TIMER_FLAG_PERIODIC) 
    {
        flag = OS_TIMER_FLAG_PERIODIC;
    } 
    else if (mpy_flag == MP_TIMER_FLAG_ONE_SHOT ) 
    {
        flag = OS_TIMER_FLAG_ONE_SHOT;
    }

    return flag;
}

static int timer_ctrl(void *device, int cmd, void* arg)
{
    machine_timer_obj_t *dev = device;
    uint8_t flag = 0;
    int32_t ret = OS_ERROR;
    
    switch (cmd)
    {
    case MP_MACHINE_OP_ENABLE:
        ret = timer_enable(device, 0);
        break;
    case MP_MACHINE_OP_DISABLE:
        ret = timer_disable(device);
        break;
    case MP_MACHINE_OP_CREATE:
        flag = timer_get_os_timer_flag(dev->timer->owner.flag);
        mp_log("Create timer{dev_name:%s, timeout:%d, flag:%d.}.",
                dev->dev_name, dev->timeout, flag);
        dev->timer->other = os_timer_create(dev->dev_name, (fun_0_1_t)arg, dev, dev->timeout, flag);
        if (NULL != dev->timer->other)
        {
            ret = OS_EOK;
        }
        break;
    case MP_MACHINE_OP_DELETE:
        ret = os_timer_destroy(dev->timer->other);
        break;
    }
    
    return ret;
}

STATIC struct operate usr_timer_ops = {
    .ioctl = timer_ctrl,
};

static int timer_register(void)
{
    MP_SINGLE_DEVICE_REGISTER(timer, DEV_TIMER, &usr_timer_ops);
    return MP_EOK;
}
OS_DEVICE_INIT(timer_register, OS_INIT_SUBLEVEL_LOW);

#endif

