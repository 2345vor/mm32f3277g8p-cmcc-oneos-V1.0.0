
#include "py/mperrno.h"

#if (MICROPY_PY_MACHINE_PWM)

#include "usr_misc.h"
#include "usr_pwm.h"
#include "usr_general.h"

/**
 *********************************************************************************************************
 *                                      pwm的ioctl函数
 *
 * @description: 调用此函数，可以实现打开和关闭pwm,可以实现周期和占空比的获取和设置
 *
 * @param 	  : device      通用设备结构体
 *
 *               cmd	        操作参数
 *
 *               arg        通过arg将所有需要的参数进行打包传入底层pwm驱动函数
 *
 *
 * @return     : 0           成功
 *
 *             ：-1或者其他负值  失败
 *
 * @note       : 这个函数是与machine_pwm.c对接的函数，因为micropython中对pwm的设置是频率和占空比数值，而设置到pwm驱动层
 *
 *               对应的是周期和占空比，均以ns为单位，所以此函数对此作了换算。
 *
 * @example    : 对内函数，无example
 *********************************************************************************************************
*/
static int pwm_ctrl(void *device, int cmd, void* arg)
{
    int ret = OS_ERROR;

    os_device_t *pwm_device = os_device_find(((device_info_t *)device)->owner.name);
    if (!pwm_device)
    {
        mp_err("Find PWM device failed!");
        return -MP_ENXIO;
    }

    switch (cmd)
    {
    case MPY_PWM_CMD_SET_PERIOD:
        ret = os_device_control(pwm_device, OS_PWM_CMD_SET_PERIOD, arg);
        break;
    case MP_MACHINE_OP_ENABLE:
        ret = os_device_control(pwm_device, OS_PWM_CMD_ENABLE, arg);
        break;
    case MP_MACHINE_OP_DISABLE:
        ret = os_device_control(pwm_device, OS_PWM_CMD_DISABLE, arg);
        break;
    }

    if (OS_EOK != ret)
    {
        mp_err("PWM device DO cmd[%d] failed[%d].", cmd, ret);
    }
    return ret;
}

//设置占空比
static int pwm_write(const char * dev_name, uint32_t offset, void *buf, uint32_t bufsize)
{
    return mpy_usr_driver_write(dev_name, offset, buf, bufsize, MP_USR_DRIVER_WRITE_NONBLOCK);
}

static struct operate pwm_ops = {
    .open = mpy_usr_driver_open,
    .write = pwm_write,
    .ioctl = pwm_ctrl,
    .close = mpy_usr_driver_close,
};

static int pwm_register(void)
{
    MP_SIMILAR_DEVICE_REGISTER(MICROPYTHON_MACHINE_PWM_PRENAME, DEV_BUS, &pwm_ops);
    return MP_EOK;
}

OS_DEVICE_INIT(pwm_register, OS_INIT_SUBLEVEL_LOW);
#endif  // MICROPY_PY_MACHINE_PWM
