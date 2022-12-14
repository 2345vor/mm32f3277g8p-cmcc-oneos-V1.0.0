/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * modified from Copyright (c) 2018 SummerGift <zhangyuan@rt-thread.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "machine_i2c.h"
#include "usr_i2c.h"

#ifdef MICROPY_PY_MACHINE_I2C

STATIC const mp_obj_type_t machine_hard_i2c_type;




STATIC void machine_hard_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print,"I2C(%s)\n",
            self->i2c_bus->owner.name);
    return;
}

int machine_hard_i2c_readfrom(mp_obj_base_t *self_in, uint16_t addr, uint8_t *dest, size_t len, bool stop) {
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
	return ((device_info_t *)(self->i2c_bus))->ops->read(self->i2c_bus->owner.name, addr, dest, len);
}

int machine_hard_i2c_writeto(mp_obj_base_t *self_in, uint16_t addr, const uint8_t *src, size_t len, bool stop) {
    uint8_t buf[1] = {0};
    machine_hard_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (len == 0){
        len = 1;
        if (src == NULL){
            src = buf;
        }
		return ((device_info_t *)(self->i2c_bus))->ops->write(self->i2c_bus->owner.name, addr, (void*)src, len);
    } else if (src == NULL){
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "buf must not NULL"));
    }
    return ((device_info_t *)(self->i2c_bus))->ops->write(self->i2c_bus->owner.name, addr, (void*)src, len);
}

/******************************************************************************/
/* MicroPython bindings for machine API                                       */

mp_obj_t machine_hard_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    char iic_device[IIC_NAME_MAX];

    snprintf(iic_device, sizeof(iic_device), MICROPYTHON_MACHINE_I2C_PRENAME"%d", mp_obj_get_int(all_args[0]));
    
	void * i2c_bus = mpycall_device_find(iic_device);
    if (!i2c_bus) {
			nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "I2C(%s) doesn't exist", iic_device));
    }
	((device_info_t *)(i2c_bus))->ops->open(((device_info_t *)(i2c_bus))->owner.name);

	//mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

		
    // create new hard I2C object
    machine_hard_i2c_obj_t *self = m_new_obj(machine_hard_i2c_obj_t);
    self->base.type = &machine_hard_i2c_type;
    self->i2c_bus = i2c_bus;
    return (mp_obj_t) self;
}

STATIC const mp_machine_i2c_p_t machine_hard_i2c_p = {
    .start = NULL,
    .stop = NULL,
    .read = NULL,
    .write = NULL,
    .readfrom = machine_hard_i2c_readfrom,
    .writeto = machine_hard_i2c_writeto,
};

STATIC const mp_obj_type_t machine_hard_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = machine_hard_i2c_print,
    .make_new = machine_hard_i2c_make_new,
    .protocol = &machine_hard_i2c_p,
    .locals_dict = (mp_obj_dict_t*)&mp_machine_soft_i2c_locals_dict,
};

#endif // MICROPY_PY_MACHINE_I2C
