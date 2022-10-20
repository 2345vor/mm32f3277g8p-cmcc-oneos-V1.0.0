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
 * @file        drv_usart.c
 *
 * @brief       This file implements usart driver for mm32
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <board.h>
#include <os_memory.h>
#include <bus/bus.h>

#define DRV_EXT_LVL DBG_EXT_INFO
#define DRV_EXT_TAG "drv.usart"
#include <drv_log.h>

#include "bsp.h"
#include "drv_usart.h"
#include "mm32_hal.h"

struct mm32_uart
{
    struct os_serial_device serial_dev;
    UART_TypeDef *huart;

    os_size_t count;
    os_uint8_t * buffer;

    os_size_t size;

    os_int32_t state;

    os_list_node_t list;
};

static os_list_node_t mm32_uart_list = OS_LIST_INIT(mm32_uart_list);

static os_err_t mm32_configure(struct os_serial_device *serial, struct serial_configure *cfg)
{
    #if 0

    struct mm32_uart *uart;
    UART_InitTypeDef UART_InitStructure;
    UART_HandleTypeDef *m_uart;
    os_uint32_t        data_bits;
    OS_ASSERT(serial != OS_NULL);
    OS_ASSERT(cfg != OS_NULL);


    uart = os_container_of(serial, struct mm32_uart, serial);

    UART_InitStructure.UART_BaudRate     = cfg->baud_rate;	
    UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_None;
    UART_InitStructure.UART_Mode = UART_Mode_Rx | UART_Mode_Tx;	  

    m_uart = uart->huart;
    switch (cfg->stop_bits)
    {
        case STOP_BITS_1:
            UART_InitStructure.UART_StopBits = UART_StopBits_1;
            break;
        case STOP_BITS_2:
            UART_InitStructure.UART_StopBits = UART_StopBits_2;
            break;
        default:
            return OS_EINVAL;
    }
    switch (cfg->parity)
    {
        case PARITY_NONE:
            UART_InitStructure.UART_Parity = UART_Parity_No;
            data_bits                = cfg->data_bits;
            break;
        default:
            return OS_EINVAL;
    }

    switch (data_bits)
    {
        case DATA_BITS_8:
            UART_InitStructure.UART_WordLength = UART_WordLength_8b;
            break;    
        default:
            return OS_EINVAL;
    }

    UART_Init(m_uart->dev, &UART_InitStructure);		

    UART_ITConfig(m_uart->dev, UART_IT_RXIEN, ENABLE);

    UART_Cmd(m_uart->dev, ENABLE);  

    NVIC_EnableIRQ(uart->huart->irq);	
    #endif
    return OS_EOK;
    
}

void mm32_uart_dma_send_cfg(uint32_t rcc_clk, UART_TypeDef* uart, DMA_Channel_TypeDef* dam_chx, u32 cpar, u32 cmar, u16 cndtr)
{
    DMA_InitTypeDef DMA_InitStruct;

    RCC_AHBPeriphClockCmd(rcc_clk, ENABLE);

    DMA_DeInit(dam_chx);

    DMA_StructInit(&DMA_InitStruct);
    //DMA transfer peripheral address
    DMA_InitStruct.DMA_PeripheralBaseAddr = cpar;
    //DMA transfer memory address
    DMA_InitStruct.DMA_MemoryBaseAddr = cmar;
    //DMA transfer direction from peripheral to memory
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
    //DMA cache size
    DMA_InitStruct.DMA_BufferSize = cndtr;
    //After receiving the data, the peripheral address is forbidden to move
    //backward
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    //After receiving the data, the memory address is shifted backward
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    //Define the peripheral data width to 8 bits
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
    //M2M mode is disabled
    DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
    DMA_InitStruct.DMA_Auto_reload = DMA_Auto_Reload_Disable;
    DMA_Init(dam_chx, &DMA_InitStruct);

    // Enable UARTy_DMA1_Channel Transfer complete interrupt
    DMA_ITConfig(dam_chx, DMA_IT_TC, ENABLE);

    UART_DMACmd(uart, UART_GCR_DMA, ENABLE);
    // UARTy_DMA1_Channel enable
    DMA_Cmd(dam_chx, ENABLE);

}

void NVIC_Configure(u8 ch, u8 pri, u8 sub)
{
    exNVIC_Init_TypeDef  NVIC_InitStruct;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStruct.NVIC_IRQChannel = ch;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = pri;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = sub;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;

    exNVIC_Init(&NVIC_InitStruct);
}

static int mm32_uart_start_send(struct os_serial_device *serial, const os_uint8_t *buff, os_size_t size)
{   
    struct mm32_uart *uart;

    OS_ASSERT(serial != OS_NULL);

    uart = os_container_of(serial, struct mm32_uart, serial_dev);    

    OS_ASSERT(uart != OS_NULL);
    
    if (uart->huart == UART6)
    {
        mm32_uart_dma_send_cfg(RCC_AHBENR_DMA2, uart->huart, DMA2_Channel4, (u32)&uart->huart->TDR, (u32)buff, size);
        NVIC_Configure(DMA2_Channel4_IRQn, 2, 0);
    }
    
    return size;
}

static int mm32_uart_stop_send(struct os_serial_device *serial)
{
    struct mm32_uart *uart;

    OS_ASSERT(serial != OS_NULL);

    uart = os_container_of(serial, struct mm32_uart, serial_dev);    

    OS_ASSERT(uart != OS_NULL);
    
    if (uart->huart == UART6)
    {
        // UARTy_DMA1_Channel enable
        DMA_Cmd(DMA2_Channel4, DISABLE);
        UART_DMACmd(uart->huart, UART_DMAReq_EN, DISABLE);
        // Enable UARTy_DMA1_Channel Transfer complete interrupt
        DMA_ITConfig(DMA2_Channel4, DMA_IT_TC, DISABLE);
    }
    
    return 0;
}






void mm32_uart_dma_rcv_cfg(uint32_t rcc_clk, UART_TypeDef* uart, DMA_Channel_TypeDef* dam_chx, u32 cpar, u32 cmar, u16 cndtr)
{
    DMA_InitTypeDef DMA_InitStruct;

    RCC_AHBPeriphClockCmd(rcc_clk, ENABLE);

    DMA_DeInit(dam_chx);
    DMA_StructInit(&DMA_InitStruct);
    //DMA transfer peripheral address
    DMA_InitStruct.DMA_PeripheralBaseAddr = cpar;
    //DMA transfer memory address
    DMA_InitStruct.DMA_MemoryBaseAddr = cmar;
    //DMA transfer direction from peripheral to memory
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
    //DMA cache size
    DMA_InitStruct.DMA_BufferSize = cndtr;
    //After receiving the data, the peripheral address is forbidden to move
    //backward
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    //After receiving the data, the memory address is shifted backward
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    //Define the peripheral data width to 8 bits
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStruct.DMA_Priority = DMA_Priority_Low;
    //M2M mode is disabled
    DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
    DMA_InitStruct.DMA_Auto_reload = DMA_Auto_Reload_Disable;
    DMA_Init(dam_chx, &DMA_InitStruct);

    // Enable UARTy_DMA1_Channel Transfer complete interrupt
    DMA_ITConfig(dam_chx, DMA_IT_TC, ENABLE);

    UART_DMACmd(uart, UART_DMAReq_EN, ENABLE);
    // UARTy_DMA1_Channel enable
    DMA_Cmd(dam_chx, ENABLE);
    
    
}

static int mm32_uart_start_recv(struct os_serial_device *serial, os_uint8_t *buff, os_size_t size)
{
    struct mm32_uart *uart;

    OS_ASSERT(serial != OS_NULL);

    uart = os_container_of(serial, struct mm32_uart, serial_dev);    

    OS_ASSERT(uart != OS_NULL);

    uart->count = 0;
    uart->buffer = buff;
    uart->size = size;
    uart->state = SET;
    
    if (uart->huart == UART6)
    {
        mm32_uart_dma_rcv_cfg(RCC_AHBENR_DMA1, uart->huart, DMA1_Channel1, (u32)&uart->huart->RDR, (u32)buff, size);
        NVIC_Configure(DMA1_Channel1_IRQn, 2, 0);
        UART_ITConfig(uart->huart, UART_ISR_RXIDLE, ENABLE);
    }
    else
    {
        UART_ITConfig(uart->huart, UART_IER_RX, ENABLE);
        UART_ITConfig(uart->huart, UART_ISR_RXIDLE, ENABLE);
    }

    return OS_EOK;
}

static int mm32_uart_stop_recv(struct os_serial_device *serial)
{
    struct mm32_uart *uart;

    OS_ASSERT(serial != OS_NULL);

    uart = os_container_of(serial, struct mm32_uart, serial_dev);

    OS_ASSERT(uart != OS_NULL);

    uart->count = 0;
    uart->buffer = OS_NULL;
    uart->state = RESET;

    if (uart->huart == UART6)
    {
        // UARTy_DMA1_Channel enable
        DMA_Cmd(DMA1_Channel1, DISABLE);
        UART_DMACmd(UART6, UART_DMAReq_EN, DISABLE);
        // Enable UARTy_DMA1_Channel Transfer complete interrupt
        DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, DISABLE);

//        DMA_DeInit(DMA1_Channel1);
    }
    else
    {
        UART_ITConfig(uart->huart, UART_IER_RX, DISABLE);
        UART_ITConfig(uart->huart, UART_ISR_RXIDLE, DISABLE);
    }

    return OS_EOK;
}

static int mm32_uart_poll_send(struct os_serial_device *serial, const os_uint8_t *buff, os_size_t size)
{
    struct mm32_uart *uart;	

    int i;
    os_base_t level;

    OS_ASSERT(serial != OS_NULL);

    uart = os_container_of(serial, struct mm32_uart, serial_dev);

    if (uart->huart == UART6)
    {
        i = 0;
    }
    for (i = 0; i < size; i++)
    {
        level = os_irq_lock();	
        UART_SendData(uart->huart, *(buff + i));
        while(!UART_GetFlagStatus(uart->huart, UART_FLAG_TXEPT));
        os_irq_unlock(level);
    }

    return size;
}

static const struct os_uart_ops mm32_uart_ops = 
{
    .init    = mm32_configure,

    .start_send   = mm32_uart_start_send,
    .stop_send    = mm32_uart_stop_send,

    .start_recv   = mm32_uart_start_recv,
    .stop_recv    = mm32_uart_stop_recv,

    .poll_send    = mm32_uart_poll_send,
};
static void uart_isr(UART_TypeDef *huart)
{    
    struct mm32_uart *uart;
    os_base_t level;

    os_list_for_each_entry(uart, &mm32_uart_list, struct mm32_uart, list)
    {
        if (uart->huart != huart)
            continue;
        
        if (RESET != UART_GetITStatus(uart->huart, UART_ISR_RX)) 
        {
            level = os_irq_lock();
            /* receive data */
            
            *(uart->buffer + uart->count) = UART_ReceiveData(uart->huart);
            UART_ClearITPendingBit(uart->huart, UART_ISR_RX);
            
            uart->count++;
            if(uart->count == uart->size)
            {
                UART_ITConfig(uart->huart, UART_IER_RX, DISABLE);
                UART_ITConfig(uart->huart, UART_ISR_RXIDLE, DISABLE);
                os_hw_serial_isr_rxdone((struct os_serial_device *)uart, uart->count);
            }
            
            os_irq_unlock(level);
        }
        
        else if(RESET != UART_GetITStatus(uart->huart, UART_ISR_RXIDLE))
        {
            level = os_irq_lock();
            //read to clear idle UART_ISR_RXIDLE
            UART_ReceiveData(uart->huart);
            UART_ClearITPendingBit(uart->huart, UART_ISR_RXIDLE);
            if (uart->count != 0)
            {
                UART_ITConfig(uart->huart, UART_IER_RX, DISABLE);
                UART_ITConfig(uart->huart, UART_ISR_RXIDLE, DISABLE);
                os_hw_serial_isr_rxdone((struct os_serial_device *)uart, uart->count);
            }
            os_irq_unlock(level);
        }

    }
}

#if defined(BSP_USING_UART1)
/* UART1 device driver structure */
void UART1_IRQHandler(void)
{
    uart_isr(UART1);
}
#endif /* BSP_USING_UART1 */

#if defined(BSP_USING_UART6)
/* UART6 device driver structure */
void UART6_IRQHandler(void)
{
    struct mm32_uart *uart;
    os_base_t level;

    os_list_for_each_entry(uart, &mm32_uart_list, struct mm32_uart, list)
    {
        if (uart->huart == UART6)
        {
            if(RESET != UART_GetITStatus(uart->huart, UART_ISR_RXIDLE))
            {
                
                level = os_irq_lock();
                UART_ReceiveData(uart->huart);
                UART_ClearITPendingBit(uart->huart, UART_ISR_RXIDLE);
                
                uart->count = uart->size - DMA_GetCurrDataCounter(DMA1_ch1);
                
                if (uart->count != 0)
                {
                    UART_ITConfig(uart->huart, UART_ISR_RXIDLE, DISABLE);
                    DMA_Cmd(DMA1_Channel1, DISABLE);
                    //close dma rcv
                    UART_DMACmd(UART6, UART_DMAReq_EN, DISABLE);
                    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, DISABLE);
                    os_hw_serial_isr_rxdone((struct os_serial_device *)uart, uart->count);
                }
                os_irq_unlock(level);
            }
        }
    }
}

// uart6 tx
void DMA2_Channel4_IRQHandler(void)
{
    struct mm32_uart *uart;
    os_base_t level;
    
    if(DMA_GetITStatus(DMA2_IT_TC4)) 
    {
        DMA_ClearITPendingBit(DMA2_IT_TC4);
        DMA_Cmd(DMA2_Channel4, DISABLE);
        // Check the received buffer
        os_list_for_each_entry(uart, &mm32_uart_list, struct mm32_uart, list)
        {
            if (uart->huart == UART6)
            {
                /* enter interrupt */
                level = os_irq_lock();
				OS_ASSERT(DMA_GetCurrDataCounter(DMA2_ch4) == 0);
                os_hw_serial_isr_txdone((struct os_serial_device *)uart);
                os_irq_unlock(level);
                break;
            }
        }
    }
    
    

}

// uart6 rx
void DMA1_Channel1_IRQHandler(void)
{
    struct mm32_uart *uart;
    os_base_t level;
    
    if(DMA_GetITStatus(DMA1_IT_TC1)) 
    {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        DMA_Cmd(DMA1_Channel1, DISABLE);
        // Check the received buffer
        os_list_for_each_entry(uart, &mm32_uart_list, struct mm32_uart, list)
        {
            if (uart->huart == UART6)
            {
                /* enter interrupt */
                level = os_irq_lock();
                /* disable the USART idle interrupt */
                UART_ITConfig(uart->huart, UART_ISR_RXIDLE, DISABLE);
				OS_ASSERT(DMA_GetCurrDataCounter(DMA1_ch1) == 0);
                uart->count = uart->size;
                os_hw_serial_isr_rxdone((struct os_serial_device *)uart, uart->count);
                uart->state = RESET;
                os_irq_unlock(level);
                break;
            }
        }
    }
}


#endif /* BSP_USING_UART2 */
void nvic_irq_enable(uint32_t irq)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_InitStructure.NVIC_IRQChannel = irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
    NVIC_Init(&NVIC_InitStructure);
}


static void mm32_uart_gpio_init(struct mm32_uart_info *uart_info)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_AHBPeriphClockCmd(uart_info->pin_clk, ENABLE); 
    
    GPIO_PinAFConfig(uart_info->pin_port, uart_info->tx_pin_source, uart_info->gpio_af_idx);
    GPIO_PinAFConfig(uart_info->pin_port, uart_info->rx_pin_source, uart_info->gpio_af_idx);
    /* Configure USART Rx/tx PIN */
    GPIO_InitStructure.GPIO_Pin = uart_info->tx_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(uart_info->pin_port, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = uart_info->rx_pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(uart_info->pin_port, &GPIO_InitStructure);
}

void uart_pheri_init(struct mm32_uart_info *uart_info)
{
    UART_InitTypeDef UART_InitStruct;

    //Baud rate
    UART_StructInit(&UART_InitStruct);
    UART_InitStruct.BaudRate = uart_info->baud_rate;
    //The word length is in 8-bit data format.
    UART_InitStruct.WordLength = UART_WordLength_8b;
    UART_InitStruct.StopBits = UART_StopBits_1;
    //No even check bit.
    UART_InitStruct.Parity = UART_Parity_No;
    //No hardware data flow control.
    UART_InitStruct.HWFlowControl = UART_HWFlowControl_None;
    UART_InitStruct.Mode = UART_Mode_Rx | UART_Mode_Tx;

    UART_Init(uart_info->huart, &UART_InitStruct);
//    UART_ITConfig(uart_info->huart, UART_IT_TXIEN|UART_IT_RXIEN, ENABLE);

    UART_Cmd(uart_info->huart, ENABLE);
}

void mm32_uart_hardware_init(struct mm32_uart_info *uart_info)
{
    uart_info->uart_rcc_func(uart_info->uart_clk, ENABLE);
    mm32_uart_gpio_init(uart_info);
    uart_pheri_init(uart_info);
}

static void mm32_uart_init(struct mm32_uart_info *uart_info)
{
    
    nvic_irq_enable(uart_info->irq);
    mm32_uart_hardware_init(uart_info);
}

static int mm32_usart_probe(const os_driver_info_t *drv, const os_device_info_t *dev)
{
    struct serial_configure config  = OS_SERIAL_CONFIG_DEFAULT;

    os_err_t    result  = 0;
    os_base_t   level;

    struct mm32_uart *uart = os_calloc(1, sizeof(struct mm32_uart));	
    

    OS_ASSERT(uart);
    struct mm32_uart_info * uart_info = (struct mm32_uart_info *)dev->info;
    uart->huart = (UART_TypeDef *)uart_info->huart;

    //usart init
    mm32_uart_init(uart_info);
    
    struct os_serial_device *serial = &uart->serial_dev;	

    serial->ops    = &mm32_uart_ops;
    serial->config = config;				

    level = os_irq_lock();
    os_list_add_tail(&mm32_uart_list, &uart->list);
    os_irq_unlock(level);

    result = os_hw_serial_register(serial, dev->name, NULL);    

    OS_ASSERT(result == OS_EOK);

    return result;
}

static UART_TypeDef * console_uart = 0;//&huart1;
void __os_hw_console_output(char *str)
{
    if (console_uart == 0)
        return;

    while (*str)
    {
        if (*str == '\n')
        {
            UART_SendData(console_uart, '\r');
            while(!UART_GetFlagStatus(console_uart, UART_FLAG_TXEPT));
        }
        UART_SendData(console_uart, *str);
        while(!UART_GetFlagStatus(console_uart, UART_FLAG_TXEPT));
        str++;
    }

}


OS_DRIVER_INFO mm32_usart_driver = {
    .name   = "UART_HandleTypeDef",
    .probe  = mm32_usart_probe,
};

OS_DRIVER_DEFINE(mm32_usart_driver, PREV, OS_INIT_SUBLEVEL_HIGH);
#include "string.h"
static int mm32_uart_early_probe(const os_driver_info_t *drv, const os_device_info_t *dev)
{
    if(!strcmp(dev->name, OS_CONSOLE_DEVICE_NAME))
    {
        struct mm32_uart_info * uart_info = (struct mm32_uart_info *)dev->info;
        mm32_uart_init(uart_info);
        console_uart = (UART_TypeDef *)uart_info->huart;
    }
    return OS_EOK;
}

OS_DRIVER_INFO mm32_uart_early_driver = {
    .name   = "UART_HandleTypeDef",
    .probe  = mm32_uart_early_probe,
};

OS_DRIVER_DEFINE(mm32_uart_early_driver, CORE, OS_INIT_SUBLEVEL_MIDDLE);

