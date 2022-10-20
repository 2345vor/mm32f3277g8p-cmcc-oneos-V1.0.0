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
 * @file        drv_usart.h
 *
 * @brief        This file provides functions declaration for usart driver.
 *
 * @revision
 * Date         Author          Notes
 * 2021-04-12   OneOS Team      First Version
 ***********************************************************************************************************************
 */
 
 
#ifndef __DRV_USART_H__
#define __DRV_USART_H__

#include <device.h>
#include "types.h"

struct mm32_uart_info 
{
    void *      huart;
    os_uint32_t uart_clk;
    os_uint32_t baud_rate;
    os_uint32_t irq;
    os_uint32_t tx_pin;
    os_uint32_t tx_pin_source;
    os_uint32_t rx_pin;
    os_uint32_t rx_pin_source;
    void *      pin_port;
    os_uint32_t pin_clk;
    os_uint32_t gpio_af_idx;
    void(*uart_rcc_func)(os_uint32_t x, FunctionalState y);
};




#endif /* __DRV_USART_H__ */
 
 
 
