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
 * @file        drv_common.c
 *
 * @brief       This file provides systick time init/IRQ and board init functions.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <oneos_config.h>
#include <os_types.h>
#include <os_stddef.h>
#include <os_clock.h>
#include <os_memory.h>
#include "drv_common.h"
#include "board.h"
#include "drv_gpio.h"
//#include "drv_sdram.h"

#ifdef OS_USING_CLOCKSOURCE
#include <timer/clocksource.h>
#include <timer/clocksource_cortexm.h>
#endif

#include <timer/hrtimer.h>
#ifdef OS_USING_FAL
#include <fal_cfg.h>
#endif

static volatile os_bool_t hardware_init_done = OS_FALSE;



void SysTick_Handler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();

    os_tick_increase();

#ifdef OS_USING_CLOCKSOURCE	
    os_clocksource_update();
#endif
    /* leave interrupt */
    os_irq_unlock(level);
}
void HAL_SuspendTick(void)
{
    /* Disable SysTick Interrupt */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;
}

void _Error_Handler(char *s, int num)
{
    volatile int loop = 1;
    while (loop);
}

int hardware_init(void);


/**
 ***********************************************************************************************************************
 * @brief           This function will initial STM32 board.
 *
 * @param[in]       none
 *
 * @return          none
 ***********************************************************************************************************************
 */

static os_err_t os_hw_board_init(void)
{

#ifdef OS_USE_BOOTLOADER
#ifdef SOC_SERIES_STM32F0
    memcpy((void*)0x20000000, (void*)USER_APP_ENTRY, 0xBC);
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SRAM();
#else
    SCB->VTOR = USER_APP_ENTRY;
#endif
#endif

#ifdef SCB_EnableICache
    /* Enable I-Cache---------------------------------------------------------*/
    SCB_EnableICache();
#endif

#ifdef SCB_EnableDCache
    /* Enable D-Cache---------------------------------------------------------*/
    SCB_EnableDCache();
#endif

    /* hardware init start, enable irq for systick */
    /* some hardware may init timeout */

    hardware_init_done = OS_FALSE;

    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();

    hardware_init(); 

    /* leave interrupt */
    os_irq_unlock(level);

    hardware_init_done = OS_TRUE;

    /* hardware init end, disable irq */



#ifdef HAL_SDRAM_MODULE_ENABLED
    SDRAM_Init();
#endif

    /* Heap initialization */
    #define OS_USING_HEAP
#if defined(OS_USING_HEAP)
    os_sys_heap_init();
    os_sys_heap_add((void *)HEAP_BEGIN, (os_size_t)HEAP_END - (os_size_t)HEAP_BEGIN, OS_MEM_ALG_DEFAULT);
//    os_system_heap_init((void *)HEAP_BEGIN, (void *)HEAP_END);
#endif

   // os_dma_mem_init();

#if defined(OS_USING_CLOCKSOURCE_CORTEXM) && defined(DWT)
    cortexm_dwt_init();
#endif

//    os_board_auto_init();	
    return OS_EOK;
}

OS_CORE_INIT(os_hw_board_init, OS_INIT_SUBLEVEL_HIGH);

static os_err_t board_post_init(void)
{
    /* Pin driver initialization is open by default */
#ifdef OS_USING_PIN
    os_hw_pin_init();
#endif
    
    /* USART driver initialization is open by default */
#ifdef OS_USING_SERIAL
//    os_hw_usart_init();
#endif
#if defined(OS_USING_SYSTICK_FOR_CLOCKSOURCE) && defined(DWT)
    cortexm_dwt_init();
#endif
    return OS_EOK;
}

OS_POSTCORE_INIT(board_post_init, OS_INIT_SUBLEVEL_HIGH);
