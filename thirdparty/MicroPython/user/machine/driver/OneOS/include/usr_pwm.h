#ifndef _USR_PWM_H_
#define _USR_PWM_H_
#include "py/obj.h"
#include "model_device.h"
#include "mpconfigport.h"
#include "model_bus.h"
#include "modmachine.h"

#if (MICROPY_PY_MACHINE_PWM)
#include "pwm.h"
#endif
extern const mp_obj_type_t mp_module_pwm;

#define MPY_PWM_CMD_SET_PERIOD      1

#endif /* _USR_PWM_H_ */

