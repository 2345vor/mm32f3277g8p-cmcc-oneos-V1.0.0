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
 * @file        os_readyq.c
 *
 * @brief       This file implements the sched functions.
 *
 * @revision
 * Date         Author          Notes
 * 2020-10-13   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <oneos_config.h>
#include <os_types.h>
#include <os_stddef.h>
#include <os_assert.h>
#include <os_errno.h>
#include <os_spinlock.h>
#include <arch_interrupt.h>
#include "os_kernel_internal.h"


#ifdef OS_CONFIG_SMP

/* TODO: */

#else

struct kernel_lock g_os_kernel_lock = {0};

void k_kernel_enter_check(struct kernel_lock *lock)
{
#ifdef OS_USING_KERNEL_LOCK_CHECK
    if (OS_NULL != g_os_current_task)
    {
        /* Exception context. */
        if (os_is_fault_active())
        {
        
        }
        /* Interrupt context. */
        else if (os_is_irq_active())
        {
            OS_ASSERT_EX(lock->owner != (void *)os_irq_num(),
                         "Irq[%d] use kernl lock recursively",
                         os_irq_num());

            lock->owner = (void *)os_irq_num();
        }
        /* Task context. */
        else
        {
             OS_ASSERT_EX(lock->owner != (void *)g_os_current_task,
                          "Task[%s] use kernl lock recursively",
                          g_os_current_task->name);

            lock->owner = (void *)g_os_current_task;
        }
    }
#endif

    return;
}

void k_kernel_exit_check(struct kernel_lock *lock)
{
#ifdef OS_USING_KERNEL_LOCK_CHECK
    if (g_os_current_task)
    {
        /* Exception context. */
        if(os_is_fault_active())
        {
        
        }
        /* Interrupt context. */
        else if (os_is_irq_active())
        {
            OS_ASSERT_EX(lock->owner == (void *)os_irq_num(),
                         "Kernl lock owner[irq %d] is not current[irq %d]",
                         lock->owner, os_irq_num());

            lock->owner = OS_NULL;
        }
        /* Task context. */
        else
        {
            OS_ASSERT_EX(lock->owner == (void *)g_os_current_task,
                         "Kernl lock owner[%s] is not current task[%s]",
                         (OS_NULL == lock->owner) ? "None" : ((os_task_t *)lock->owner)->name,
                         g_os_current_task->name);

            lock->owner = OS_NULL;
        }
    }
#endif

    return;
}
#endif /* OS_CONFIG_SMP */

