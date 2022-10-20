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
 * @file        cpuport.c
 *
 * @brief       This file provides functions related to the ARM M4 architecture.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-23   OneOS Team      First version.
 ***********************************************************************************************************************
 */
#include <oneos_config.h>
#include <os_types.h>
#include <os_stddef.h>

volatile os_ubase_t uxCriticalNesting;

os_ubase_t os_irq_lock(void)
{
    if ( uxCriticalNesting == 0 )
    {
        disable_interrupts();
    }
    uxCriticalNesting++;
        
    return uxCriticalNesting;
}

void os_irq_unlock(os_ubase_t irq_save)
{
    uxCriticalNesting--;

    /* If we have reached 0 then re-enable the interrupts. */
    if( uxCriticalNesting == 0 )
    {
        enable_interrupts();
    }
}

void os_irq_disable(void)
{

}

void os_irq_enable(void)
{

}

os_bool_t os_is_irq_active(void)
{
    return 0;
}

os_bool_t os_is_irq_disabled(void)
{
    return 0;
}

os_uint32_t os_irq_num(void)
{
    return 0;
}

os_bool_t os_is_fault_active(void)
{
    return 0;
}


