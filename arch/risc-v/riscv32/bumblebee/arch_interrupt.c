#include <os_types.h>
#include "riscv_encoding.h"
#include "riscv_bits.h"

/**
 ***********************************************************************************************************************
 * @brief           Disable interrupt.
 *
 * @param           None.
 *
 * @return          The state before disable interrupt.
 ***********************************************************************************************************************
 */
os_ubase_t os_irq_lock(void)
{
    __volatile__ os_ubase_t mie;

    __asm__ __volatile__(
        "csrrci %0, mstatus, 0x00000008\n"
        "andi %0, %0, 0x00000008\n"
        : "=r"(mie)
        : 
        : "memory");

    return mie;
}

/**
 ***********************************************************************************************************************
 * @brief           Enable interrupt.
 *
 * @param[in]       The state to restore.
 *
 * @return          None.
 ***********************************************************************************************************************
 */
void os_irq_unlock(os_ubase_t irq_save)
{
    __asm__ __volatile__(
        "csrs mstatus, %0\n"
        : 
        : "r"(irq_save)
        : "memory");

    return;
}
/**
 ***********************************************************************************************************************
 * @brief           Get the current context state, 1: irq context  0: task context.
 *
 * @param           None
 *
 * @return          0:        task context.
 *                  1:        irq context.
 ***********************************************************************************************************************
 */
os_bool_t os_is_irq_active(void)
{
    __volatile__ os_bool_t ret;
    __volatile__ os_ubase_t oldcsr;
    __volatile__ os_ubase_t tmpval;

    __asm__ __volatile__(
        "addi %0, x0, 0\n"
        "addi %2, x0, 0x00000040\n"        /* Set irq context is msubm_TYP=1 temporarily, when debug mode is a exception type. */
        "csrr %1, 0x7c4\n"
        "andi %1, %1, 0x000000c0\n"
        "bne  %1, %2, in_task_context\n"
        "addi %0, x0, 1\n"
        "in_task_context:"
        :"+r"(ret),"+r"(oldcsr),"+r"(tmpval)
        : 
        :"cc", "memory");

    return ret;
}
/**
 ***********************************************************************************************************************
 * @brief           Get irq status.
 *
 * @param           None
 *
 * @return          0:    irq enable
 *                  1:    irq disable
 ***********************************************************************************************************************
 */
os_bool_t os_is_irq_disabled(void)
{
    __volatile__ os_bool_t ret;
    __volatile__ os_ubase_t oldcsr;

    __asm__ __volatile__(
        "addi %0, x0, 0\n"
        "csrr %1, mstatus\n"
        "andi %1, %1, 0x00000008\n"
        "bnez %1, in_irq_enable\n"
        "addi %0, x0, 1\n"
        "in_irq_enable:"
        :"+r"(ret),"+r"(oldcsr)
        : 
        :"cc", "memory");

    return ret;    
}
/**
 ***********************************************************************************************************************
 * @brief           Get irq num.
 *
 * @param           None
 *
 * @return          "CSR_MCAUSE" num
 ***********************************************************************************************************************
 */
os_uint32_t os_irq_num(void)
{
    __volatile__ os_uint32_t ret;

    __asm__ __volatile__(
        "csrr %0, mcause\n"
        :"+r"(ret)
        : 
        :"memory");

    return ret;    
}
/**
 ***********************************************************************************************************************
 * @brief           Determine whether the current context is an exception context.
 *
 * @detail          If return 0, context may running into one status of these: "task", "interrupt" and "NMI". 
 *
 * @param           None.
 *
 * @return          Return 1 in exception context, otherwise return 0.
 * @retval          1               In exception context.
 * @retval          0               In other context.
 ***********************************************************************************************************************
 */
os_bool_t os_is_fault_active(void)
{
    __volatile__ os_bool_t ret;
    __volatile__ os_ubase_t oldcsr;
    __volatile__ os_ubase_t tmpval;

    __asm__ __volatile__(
        "addi %0, x0, 0\n"
        "addi %2, x0, 0x00000080\n"
        "csrr %1, 0x7c4\n"
        "andi %1, %1, 0x000000c0\n"
        "bne  %1, %2, in_other_context\n"
        "addi %0, x0, 1\n"
        "in_other_context:"
        :"+r"(ret),"+r"(oldcsr),"+r"(tmpval)
        : 
        :"cc", "memory");

    return ret;
}

