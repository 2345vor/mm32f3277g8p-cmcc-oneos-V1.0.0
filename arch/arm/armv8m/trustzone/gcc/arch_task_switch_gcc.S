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
 * @file        arch_task_switch_gcc.S
 *
 * @brief       This file provides context switch functions related to the ARMv8-M architecture, 
 *              and the compiler uses gcc.
 *
 * @revision
 * Date         Author          Notes
 * 2021-01-12   OneOS Team      First version.
 ***********************************************************************************************************************
 */
.syntax unified
.thumb
.text

.equ    SCB_VTOR,           0xE000ED08              /* Vector table offset Register. */
.equ    NVIC_INT_CTRL,      0xE000ED04              /* Interrupt control state register. */
.equ    NVIC_SYSPRI2,       0xE000ED20              /* System priority register (2). */
.equ    NVIC_PENDSV_PRI,    0x00FF0000              /* PendSV priority value (lowest). */
.equ    NVIC_PENDSVSET,     0x10000000              /* Value to trigger PendSV exception. */

.equ    OS_TASK_STATE_RUNNING,  0x0004              /* Task Status Flag (RUNNING) */
.extern g_os_current_task
.extern g_os_next_task
.extern _g_arch_tz_context_memory_id
.extern _arch_tz_context_store
.extern _arch_tz_context_load
.extern os_task_switch_fn

.global os_task_switch
.type   os_task_switch, %function

.global os_first_task_start
.type   os_first_task_start, %function

.global PendSV_Handler
.type   PendSV_Handler, %function

/**
 ***********************************************************************************************************************
 * @brief           The system starts to switch to the first task.
 *
 * @param           None
 *
 * @return          None.
 ***********************************************************************************************************************
 */
os_first_task_start:
    /*set the PendSV exception priority*/
    LDR     R0, =NVIC_SYSPRI2
    LDR     R1, =NVIC_PENDSV_PRI
    LDR.W   R2, [R0, #0x00]                      /* Read */
    ORR     R1, R1, R2                           /* Modify */
    STR     R1, [R0]                             /* Write-back */

    #if (defined (__VFP_FP__) && !defined(__SOFTFP__))
        /* Clear CONTROL.FPCA */
        MRS     R2, CONTROL             /* Read */
        BIC     R2, #0x04               /* Modify */
        MSR     CONTROL, R2             /* Write-back */
    #endif
    /* Restore MSP */
    LDR     R0, =SCB_VTOR
    LDR     R0, [R0]
    LDR     R0, [R0]
    MSR     MSP, R0

    /* Trigger the PendSV exception (causes context switch) */
    LDR     R0, =NVIC_INT_CTRL
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    /* Enable interrupts at processor level */
    CPSIE   F
    CPSIE   I

    /* Never reach here! */
    B .

/**
 ***********************************************************************************************************************
 * @brief           Trigger task context switch.
 *
 * @param           None
 *
 * @return          None.
 ***********************************************************************************************************************
 */
os_task_switch:
    /* Trigger pendSV */
    LDR     R0, =NVIC_INT_CTRL
    LDR     R1, =NVIC_PENDSVSET
    STR     R1, [R0]

    BX      LR

/**
 ***********************************************************************************************************************
 * @brief           Handle task context switch.
 *
 * @param           None
 *
 * @return          None.
 ***********************************************************************************************************************
 */
PendSV_Handler:
    /* Disable interrupt to prevent interruption of task switching process */
    MRS     R0, PRIMASK
    PUSH    {R0}
    CPSID   I

    /* If g_os_current_task is OS_NULL, skip preservation */
    LDR     R12,  =g_os_current_task                /* R12 = &g_os_current_task */
    LDR     R2,  [R12]                              /* R2 = gs_os_current_task */
    CBZ     R2,  switch_to_task                     /* If g_os_current_task is OS_NULL, os_first_task_start trigger */

    /* Whether the trustzone stack exists */
    LDR     R0,  =_g_arch_tz_context_memory_id      /* R0 = &_g_arch_tz_context_memory_id*/
    LDR     R0, [R0]                                /* R0 = *R0 */
    CBZ     R0, save_ns_context                     /* if R0 == 0, No secure context to save */

    /* Call trustzone store fun */
    PUSH    {R0, R2, R12, LR}
    BL      _arch_tz_context_store
    POP     {R0, R2, R12, LR}

    /* Whether the trustzone stack was used */
    TST     LR, #0x40                               /* Bit[6] of EXC_RETURN is 1: trustzone stack was used 0: non-trustzone stack was used to store stack frame */
    BEQ     save_ns_context

    /* Push _g_arch_tz_context_memory_id psplim lr to stack */
    MRS     R1, PSPLIM                              /* R1 = PSPLIM */
    MRS     R3, PSP                                 /* R3 = PSP */
    STMFD   R3!, {R0-R1, LR}                        /* Push to task stack */
    B       update_from_task_info

save_ns_context
    MRS     R3, PSP

    /* Save FPU registers */
    #if (defined (__VFP_FP__) && !defined(__SOFTFP__))
        TST       LR,  #0x10
        VSTMDBEQ  R3!, {D8 - D15}
    #endif

    STMFD   R3!, {R4 - R11}                         /* Push r4 - r11 register */

    /* Push g_arch_tz_context_memory_id psplim lr to stack */
    MRS     R1, PSPLIM                              /* R1 = PSPLIM */
    STMFD   R3!, {R0-R1, LR}                        /* Push to task stack */

update_from_task_info
    STR     R3,  [R2, #0]                           /* Update g_os_current_task->stack_top */

    /* Clear current task running state */
    LDR     R1,  =OS_TASK_STATE_RUNNING
    LDRH    R3,  [R2, #12]                          /* R3 = g_os_current_task->state */
    BIC     R3,  R3, R1                             /* g_os_current_task->state &= (~OS_TASK_STATE_RUNNING) */
    STRH    R3,  [R2, #12]

    /* Check either task stack during task switching. */
    PUSH    {R12}
    BL      os_task_switch_fn
    POP     {R12}

switch_to_task
    /* Get g_os_next_task->stack_top */
    LDR     R2, =g_os_next_task
    LDR     R2, [R2]                                /* Load task stack pointer */

    /* g_os_current_task = g_os_next_task */
    STR     R2,  [R12]

    LDR     R3,  [R2, #0]                           /* R3 = g_os_next_task->stack_top; Pickup task stack pointer */

    /* Pop g_arch_tz_context_memory_id psplim lr to stack */
    LDMFD   R3!, {R0-R1, LR}
    MSR     PSPLIM,  R1                             /* PSPLIM = R1 */
    LDR     R4,  =_g_arch_tz_context_memory_id      /* R4 = &_g_arch_tz_context_memory_id */
    STR     R0, [R4]                                /* *R4 = R0 */

    /* Whether the trustzone stack exists */
    CBZ     R0, load_ns_context
    PUSH    {R0, R2-R3, LR}
    BL      _arch_tz_context_load
    POP     {R0, R2-R3, LR}

    /* Whether the trustzone stack was used */
    TST     LR, #0x40                               /* Bit[6] of EXC_RETURN is 1: trustzone stack was used 0: non-trustzone stack was used to store stack frame */
    BNE     update_to_task_info

load_ns_context
    LDMFD   R3!, {R4 - R11}                         /* Pop r4 - r11 register */

    #if (defined (__VFP_FP__) && !defined(__SOFTFP__))
        TST       LR, #0x10                         /* if(!EXC_RETURN[4]) */
        VLDMIAEQ  R3!, {D8 - D15}                   /* Pop FPU register s16~s31 */
    #endif

update_to_task_info
    MSR     PSP, R3                                 /* Update stack pointer */

    /* Set task running state */
    LDR     R1,  =OS_TASK_STATE_RUNNING
    LDRH    R3,  [R2, #12]                          /* R3 = g_os_next_task->state */
    ORR     R3,  R3, R1                             /* g_os_next_task->state |= OS_TASK_STATE_RUNNING */
    STRH    R3,  [R2, #12]

    /* Restore interrupt */
    POP     {R0}
    MSR     PRIMASK, R0

    BX      LR
