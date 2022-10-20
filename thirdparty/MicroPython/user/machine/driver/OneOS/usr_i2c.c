#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "usr_i2c.h"
#include "usr_general.h"

#if MICROPY_PY_MACHINE_I2C
#include "usr_misc.h"


static int i2c_read(const char *dev_name, uint32_t offset, void *buf, uint32_t bufsize)
{
    return mpy_usr_driver_read(dev_name, offset, buf, bufsize, MP_USR_DRIVER_READ_NONBLOCK);
}

static int i2c_write(const char *dev_name, uint32_t offset, void *buf, uint32_t bufsize)
{
    return mpy_usr_driver_write(dev_name, offset, buf, bufsize, MP_USR_DRIVER_WRITE_NONBLOCK);
}

static int i2c_ctrl(void *device, int cmd, void* arg)
{
    os_device_t *i2c_device = os_device_find(((device_info_t *)device)->owner.name);
    if(NULL == i2c_device)
    {
        mp_err("i2c_ctrl find name error\n");
        return MP_ERROR;
    }

    return MP_EOK;
}

STATIC struct operate i2c_ops = {
    .open = mpy_usr_driver_open,
    .read = i2c_read,
    .write = i2c_write,
    .ioctl = i2c_ctrl,
    .close = mpy_usr_driver_close,
};

static int i2c_register(void)
{
    MP_SIMILAR_DEVICE_REGISTER(MICROPYTHON_MACHINE_I2C_PRENAME, DEV_BUS, &i2c_ops);
    return MP_EOK;
}
OS_DEVICE_INIT(i2c_register, OS_INIT_SUBLEVEL_LOW);

#endif

