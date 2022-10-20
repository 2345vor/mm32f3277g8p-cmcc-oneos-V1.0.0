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
 * @file        drv_gpio.c
 *
 * @brief       This file implements gpio driver for MM32.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include "drv_gpio.h"
#include <mm32_hal.h>


#ifdef OS_USING_PIN

#define MM32_PIN(index, rcc, gpio, gpio_index) { index, RCC_##rcc##_GPIO##gpio, GPIO##gpio, GPIO_Pin_##gpio_index, GPIO_PortSourceGPIO##gpio, GPIO_PinSource##gpio_index}
#define MM32_PIN_RESERVED {-1, 0, 0, 0, 0, 0}

static const struct pin_index pins[] = 
{
#if defined(GPIOA)
    MM32_PIN(0, AHBENR, A, 0),    MM32_PIN(1, AHBENR, A, 1),    MM32_PIN(2, AHBENR, A, 2),    MM32_PIN(3, AHBENR, A, 3),
    MM32_PIN(4, AHBENR, A, 4),    MM32_PIN(5, AHBENR, A, 5),    MM32_PIN(6, AHBENR, A, 6),    MM32_PIN(7, AHBENR, A, 7),
    MM32_PIN(8, AHBENR, A, 8),    MM32_PIN(9, AHBENR, A, 9),    MM32_PIN(10, AHBENR, A, 10),  MM32_PIN(11, AHBENR, A, 11),
    MM32_PIN(12, AHBENR, A, 12),  MM32_PIN(13, AHBENR, A, 13),  MM32_PIN(14, AHBENR, A, 14),  MM32_PIN(15, AHBENR, A, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOA) */
    
#if defined(GPIOB)
    MM32_PIN(16, AHBENR, B, 0),   MM32_PIN(17, AHBENR, B, 1),   MM32_PIN(18, AHBENR, B, 2),   MM32_PIN(19, AHBENR, B, 3),
    MM32_PIN(20, AHBENR, B, 4),   MM32_PIN(21, AHBENR, B, 5),   MM32_PIN(22, AHBENR, B, 6),   MM32_PIN(23, AHBENR, B, 7),
    MM32_PIN(24, AHBENR, B, 8),   MM32_PIN(25, AHBENR, B, 9),   MM32_PIN(26, AHBENR, B, 10),  MM32_PIN(27, AHBENR, B, 11),
    MM32_PIN(28, AHBENR, B, 12),  MM32_PIN(29, AHBENR, B, 13),  MM32_PIN(30, AHBENR, B, 14),  MM32_PIN(31, AHBENR, B, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOB) */
#if defined(GPIOC)
    MM32_PIN(32, AHBENR, C, 0),   MM32_PIN(33, AHBENR, C, 1),   MM32_PIN(34, AHBENR, C, 2),   MM32_PIN(35, AHBENR, C, 3),
    MM32_PIN(36, AHBENR, C, 4),   MM32_PIN(37, AHBENR, C, 5),   MM32_PIN(38, AHBENR, C, 6),   MM32_PIN(39, AHBENR, C, 7),
    MM32_PIN(40, AHBENR, C, 8),   MM32_PIN(41, AHBENR, C, 9),   MM32_PIN(42, AHBENR, C, 10),  MM32_PIN(43, AHBENR, C, 11),
    MM32_PIN(44, AHBENR, C, 12),  MM32_PIN(45, AHBENR, C, 13),  MM32_PIN(46, AHBENR, C, 14),  MM32_PIN(47, AHBENR, C, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOC) */
#if defined(GPIOD)
    MM32_PIN(48, AHBENR, D, 0),   MM32_PIN(49, AHBENR, D, 1),   MM32_PIN(50, AHBENR, D, 2),   MM32_PIN(51, AHBENR, D, 3),
    MM32_PIN(52, AHBENR, D, 4),   MM32_PIN(53, AHBENR, D, 5),   MM32_PIN(54, AHBENR, D, 6),   MM32_PIN(55, AHBENR, D, 7),
    MM32_PIN(56, AHBENR, D, 8),   MM32_PIN(57, AHBENR, D, 9),   MM32_PIN(58, AHBENR, D, 10),  MM32_PIN(59, AHBENR, D, 11),
    MM32_PIN(60, AHBENR, D, 12),  MM32_PIN(61, AHBENR, D, 13),  MM32_PIN(62, AHBENR, D, 14),  MM32_PIN(63, AHBENR, D, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOD) */
#if defined(GPIOE)
    MM32_PIN(64, AHBENR, E, 0),   MM32_PIN(65, AHBENR, E, 1),   MM32_PIN(66, AHBENR, E, 2),   MM32_PIN(67, AHBENR, E, 3),
    MM32_PIN(68, AHBENR, E, 4),   MM32_PIN(69, AHBENR, E, 5),   MM32_PIN(70, AHBENR, E, 6),   MM32_PIN(71, AHBENR, E, 7),
    MM32_PIN(72, AHBENR, E, 8),   MM32_PIN(73, AHBENR, E, 9),   MM32_PIN(74, AHBENR, E, 10),  MM32_PIN(75, AHBENR, E, 11),
    MM32_PIN(76, AHBENR, E, 12),  MM32_PIN(77, AHBENR, E, 13),  MM32_PIN(78, AHBENR, E, 14),  MM32_PIN(79, AHBENR, E, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOE) */
#if defined(GPIOF)
    MM32_PIN(80, AHBENR, F, 0),   MM32_PIN(81, AHBENR, F, 1),   MM32_PIN(82, AHBENR, F, 2),   MM32_PIN(83, AHBENR, F, 3),
    MM32_PIN(84, AHBENR, F, 4),   MM32_PIN(85, AHBENR, F, 5),   MM32_PIN(86, AHBENR, F, 6),   MM32_PIN(87, AHBENR, F, 7),
    MM32_PIN(88, AHBENR, F, 8),   MM32_PIN(89, AHBENR, F, 9),   MM32_PIN(90, AHBENR, F, 10),  MM32_PIN(91, AHBENR, F, 11),
    MM32_PIN(92, AHBENR, F, 12),  MM32_PIN(93, AHBENR, F, 13),  MM32_PIN(94, AHBENR, F, 14),  MM32_PIN(95, AHBENR, F, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOF) */
#if defined(GPIOG)
    MM32_PIN(96, AHBENR, G, 0),   MM32_PIN(97, AHBENR, G, 1),   MM32_PIN(98, AHBENR, G, 2),   MM32_PIN(99, AHBENR, G, 3),
    MM32_PIN(100, AHBENR, G, 4),  MM32_PIN(101, AHBENR, G, 5),  MM32_PIN(102, AHBENR, G, 6),  MM32_PIN(103, AHBENR, G, 7),
    MM32_PIN(104, AHBENR, G, 8),  MM32_PIN(105, AHBENR, G, 9),  MM32_PIN(106, AHBENR, G, 10), MM32_PIN(107, AHBENR, G, 11),
    MM32_PIN(108, AHBENR, G, 12), MM32_PIN(109, AHBENR, G, 13), MM32_PIN(110, AHBENR, G, 14), MM32_PIN(111, AHBENR, G, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOG) */
#if defined(GPIOH)
    MM32_PIN(112, AHBENR, H, 0),  MM32_PIN(113, AHBENR, H, 1),  MM32_PIN(114, AHBENR, H, 2),  MM32_PIN(115, AHBENR, H, 3),
    MM32_PIN(116, AHBENR, H, 4),  MM32_PIN(117, AHBENR, H, 5),  MM32_PIN(118, AHBENR, H, 6),  MM32_PIN(119, AHBENR, H, 7),
    MM32_PIN(120, AHBENR, H, 8),  MM32_PIN(121, AHBENR, H, 9),  MM32_PIN(122, AHBENR, H, 10), MM32_PIN(123, AHBENR, H, 11),
    MM32_PIN(124, AHBENR, H, 12), MM32_PIN(125, AHBENR, H, 13), MM32_PIN(126, AHBENR, H, 14), MM32_PIN(127, AHBENR, H, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOH) */
#if defined(GPIOI)
    MM32_PIN(128, I, 0),  MM32_PIN(129, I, 1),  MM32_PIN(130, I, 2),  MM32_PIN(131, I, 3),
    MM32_PIN(132, I, 4),  MM32_PIN(133, I, 5),  MM32_PIN(134, I, 6),  MM32_PIN(135, I, 7),
    MM32_PIN(136, I, 8),  MM32_PIN(137, I, 9),  MM32_PIN(138, I, 10), MM32_PIN(139, I, 11),
    MM32_PIN(140, I, 12), MM32_PIN(141, I, 13), MM32_PIN(142, I, 14), MM32_PIN(143, I, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOI) */
#if defined(GPIOJ)
    MM32_PIN(144, J, 0),  MM32_PIN(145, J, 1),  MM32_PIN(146, J, 2),  MM32_PIN(147, J, 3),
    MM32_PIN(148, J, 4),  MM32_PIN(149, J, 5),  MM32_PIN(150, J, 6),  MM32_PIN(151, J, 7),
    MM32_PIN(152, J, 8),  MM32_PIN(153, J, 9),  MM32_PIN(154, J, 10), MM32_PIN(155, J, 11),
    MM32_PIN(156, J, 12), MM32_PIN(157, J, 13), MM32_PIN(158, J, 14), MM32_PIN(159, J, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOJ) */
#if defined(GPIOK)
    MM32_PIN(160, K, 0),  MM32_PIN(161, K, 1),  MM32_PIN(162, K, 2),  MM32_PIN(163, K, 3),
    MM32_PIN(164, K, 4),  MM32_PIN(165, K, 5),  MM32_PIN(166, K, 6),  MM32_PIN(167, K, 7),
    MM32_PIN(168, K, 8),  MM32_PIN(169, K, 9),  MM32_PIN(170, K, 10), MM32_PIN(171, K, 11),
    MM32_PIN(172, K, 12), MM32_PIN(173, K, 13), MM32_PIN(174, K, 14), MM32_PIN(175, K, 15),
#else
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
    MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED, MM32_PIN_RESERVED,
#endif /* defined(GPIOK) */









};

static const struct pin_irq_map pin_irq_map[] = 
{
    {GPIO_Pin_0,  EXTI_Line0,  EXTI0_IRQn    },
    {GPIO_Pin_1,  EXTI_Line1,  EXTI1_IRQn    },
    {GPIO_Pin_2,  EXTI_Line2,  EXTI2_IRQn    },
    {GPIO_Pin_3,  EXTI_Line3,  EXTI3_IRQn    },
    {GPIO_Pin_4,  EXTI_Line4,  EXTI4_IRQn    },
    {GPIO_Pin_5,  EXTI_Line5,  EXTI9_5_IRQn  },
    {GPIO_Pin_6,  EXTI_Line6,  EXTI9_5_IRQn  },
    {GPIO_Pin_7,  EXTI_Line7,  EXTI9_5_IRQn  },
    {GPIO_Pin_8,  EXTI_Line8,  EXTI9_5_IRQn  },
    {GPIO_Pin_9,  EXTI_Line9,  EXTI9_5_IRQn  },
    {GPIO_Pin_10, EXTI_Line10, EXTI15_10_IRQn},
    {GPIO_Pin_11, EXTI_Line11, EXTI15_10_IRQn},
    {GPIO_Pin_12, EXTI_Line12, EXTI15_10_IRQn},
    {GPIO_Pin_13, EXTI_Line13, EXTI15_10_IRQn},
    {GPIO_Pin_14, EXTI_Line14, EXTI15_10_IRQn},
    {GPIO_Pin_15, EXTI_Line15, EXTI15_10_IRQn},

};

static struct os_pin_irq_hdr pin_irq_hdr_tab[] = {
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
    {-1, 0, OS_NULL, OS_NULL},
};
//static os_uint32_t pin_irq_enable_mask = 0;

#define ITEM_NUM(items) sizeof(items) / sizeof(items[0])

//static struct pin_pull_state pin_pulls[ITEM_NUM(pins)] = {0};

static const struct pin_index *get_pin(os_uint8_t pin)
{
    const struct pin_index *index;

    if (pin < ITEM_NUM(pins))
    {
        index = &pins[pin];
        if (index->index == -1)
            index = OS_NULL;
    }
    else
    {
        index = OS_NULL;
    }

    return index;
};

static void MM32_pin_write(struct os_device *dev, os_base_t pin, os_base_t value)
{
    const struct pin_index *index;

    index = get_pin(pin);
    if (index == OS_NULL)
    {
        return;
    }

    GPIO_WriteBit(index->gpio, index->pin, (BitAction)value);
}

static int MM32_pin_read(struct os_device *dev, os_base_t pin)
{
    int                     value;
    const struct pin_index *index;

    value = PIN_LOW;

    index = get_pin(pin);
    if (index == OS_NULL)
    {
        return value;
    }

    if (GPIO_ReadInputDataBit(index->gpio, index->pin) == Bit_RESET)
    {
        value = PIN_LOW;
    }
    else
    {
        value = PIN_HIGH;
    }  

    return value;
}

static void MM32_pin_mode(struct os_device *dev, os_base_t pin, os_base_t mode)
{
    const struct pin_index *index;
    GPIO_InitTypeDef        GPIO_InitStruct;

    index = get_pin(pin);
    if (index == OS_NULL)
    {
        return;
    }
    /* GPIO Periph clock enable */
    RCC_AHBPeriphClockCmd(index->rcc, ENABLE);
    /* Configure GPIO_InitStructure */
    GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.GPIO_Pin   = index->pin;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;   
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;


    if (mode == PIN_MODE_OUTPUT)
    {
        /* output setting */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP; 
    }
    else if (mode == PIN_MODE_INPUT)
    {
        /* input setting: not pull. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    }
    else if (mode == PIN_MODE_INPUT_PULLUP)
    {
        /* input setting: pull up. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    }
    else if (mode == PIN_MODE_INPUT_PULLDOWN)
    {
        /* input setting: pull down. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
    }
    else if (mode == PIN_MODE_OUTPUT_OD)
    {
        /* output setting: od. */
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    }
    else
    {
        /* input setting:default. */
        GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPD;
    }
   
    GPIO_Init(index->gpio, &GPIO_InitStruct);
}

OS_INLINE os_int32_t bit2bitno(os_uint32_t bit)
{
    int i;
    for (i = 0; i < 32; i++)
    {
        if ((0x01 << i) == bit)
        {
            return i;
        }
    }
    return -1;
}

OS_INLINE const struct pin_irq_map *get_pin_irq_map(os_uint32_t pinbit)
{
    os_int32_t mapindex = bit2bitno(pinbit);
    if (mapindex < 0 || mapindex >= ITEM_NUM(pin_irq_map))
    {
        return OS_NULL;
    }
    return &pin_irq_map[mapindex];
};

static os_err_t
MM32_pin_attach_irq(struct os_device *device, os_int32_t pin, os_uint32_t mode, void (*hdr)(void *args), void *args)
{
    const struct pin_index *index;
    os_base_t               level;
    os_int32_t              irqindex = -1;

    index = get_pin(pin);
    if (index == OS_NULL)
    {
        return OS_ENOSYS;
    }
    irqindex = bit2bitno(index->pin);
    if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_map))
    {
        return OS_ENOSYS;
    }

    level = os_irq_lock();
    if (pin_irq_hdr_tab[irqindex].pin == pin       &&
            pin_irq_hdr_tab[irqindex].hdr == hdr   &&
            pin_irq_hdr_tab[irqindex].mode == mode &&
            pin_irq_hdr_tab[irqindex].args == args)
    {
        os_irq_unlock(level);
        return OS_EOK;
    }
    if (pin_irq_hdr_tab[irqindex].pin != -1)
    {
        os_irq_unlock(level);
        return OS_EBUSY;
    }
    pin_irq_hdr_tab[irqindex].pin  = pin;
    pin_irq_hdr_tab[irqindex].hdr  = hdr;
    pin_irq_hdr_tab[irqindex].mode = mode;
    pin_irq_hdr_tab[irqindex].args = args;
    os_irq_unlock(level);

    return OS_EOK;
}

static os_err_t MM32_pin_dettach_irq(struct os_device *device, os_int32_t pin)
{
    const struct pin_index *index;
    os_base_t               level;
    os_int32_t              irqindex = -1;

    index = get_pin(pin);
    if (index == OS_NULL)
    {
        return OS_ENOSYS;
    }
    irqindex = bit2bitno(index->pin);
    if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_map))
    {
        return OS_ENOSYS;
    }

    level = os_irq_lock();
    if (pin_irq_hdr_tab[irqindex].pin == -1)
    {
        os_irq_unlock(level);
        return OS_EOK;
    }
    pin_irq_hdr_tab[irqindex].pin  = -1;
    pin_irq_hdr_tab[irqindex].hdr  = OS_NULL;
    pin_irq_hdr_tab[irqindex].mode = 0;
    pin_irq_hdr_tab[irqindex].args = OS_NULL;
    os_irq_unlock(level);

    return OS_EOK;
}

static os_err_t MM32_pin_irq_enable(struct os_device *device, os_base_t pin, os_uint32_t enabled)
{
    const struct pin_index   *index;
    const struct pin_irq_map *irqmap;
    os_base_t                 level;
    os_int32_t                irqindex = -1;

    GPIO_InitTypeDef  GPIO_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;

    index = get_pin(pin);
    if (index == OS_NULL)
    {
        return OS_ENOSYS;
    }

    if (enabled == PIN_IRQ_ENABLE)
    {
        irqindex = bit2bitno(index->pin);
        if (irqindex < 0 || irqindex >= ITEM_NUM(pin_irq_map))
        {
            return OS_ENOSYS;
        }

        level = os_irq_lock();

        if (pin_irq_hdr_tab[irqindex].pin == -1)
        {
            os_irq_unlock(level);
            return OS_ENOSYS;
        }

        irqmap = &pin_irq_map[irqindex];

        /* GPIO Periph clock enable */
        RCC_APB2PeriphClockCmd(index->rcc, ENABLE);

        /* Configure GPIO_InitStructure */
        GPIO_InitStructure.GPIO_Pin     = index->pin;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(index->gpio, &GPIO_InitStructure);

        NVIC_InitStructure.NVIC_IRQChannel = irqmap->irqno;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
//        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&NVIC_InitStructure);

        RCC_APB2PeriphClockCmd(RCC_APB2ENR_SYSCFG, ENABLE);
        
        SYSCFG_EXTILineConfig(index->port_source, index->pin_source);
        EXTI_InitStructure.EXTI_Line = irqmap->irqbit;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;

        switch (pin_irq_hdr_tab[irqindex].mode)
        {
        case PIN_IRQ_MODE_RISING:
           	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
            break;
        case PIN_IRQ_MODE_FALLING:
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
            break;
        case PIN_IRQ_MODE_HIGH_LEVEL:
        case PIN_IRQ_MODE_LOW_LEVEL:
        default:
            os_irq_unlock(level);
            return OS_ERROR;
        }

        EXTI_InitStructure.EXTI_LineCmd = ENABLE;
        EXTI_Init(&EXTI_InitStructure);

        os_irq_unlock(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        irqmap = get_pin_irq_map(index->pin);
        if (irqmap == OS_NULL)
        {
            return OS_ENOSYS;
        }

        EXTI_InitStructure.EXTI_Line = irqmap->irqbit;
        EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
        EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
        EXTI_InitStructure.EXTI_LineCmd = DISABLE;
        EXTI_Init(&EXTI_InitStructure);
    }
    else
    {
        return OS_ENOSYS;
    }

    return OS_EOK;
}
const static struct os_pin_ops _MM32_pin_ops = 
{
    MM32_pin_mode,
    MM32_pin_write,
    MM32_pin_read,
    MM32_pin_attach_irq,
    MM32_pin_dettach_irq,
    MM32_pin_irq_enable,
};

OS_INLINE void pin_irq_hdr(int irqno)
{
    EXTI_ClearITPendingBit(pin_irq_map[irqno].irqbit);

    if (pin_irq_hdr_tab[irqno].hdr)
    {
        pin_irq_hdr_tab[irqno].hdr(pin_irq_hdr_tab[irqno].args);
    }
}


void EXTI0_IRQHandler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();	
    pin_irq_hdr(0);
    /* leave interrupt */
    os_irq_unlock(level);
}
void EXTI1_IRQHandler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();	
    pin_irq_hdr(1);
    /* leave interrupt */
    os_irq_unlock(level);
}
void EXTI2_IRQHandler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();	
    pin_irq_hdr(2);
    /* leave interrupt */
    os_irq_unlock(level);
}
void EXTI3_IRQHandler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();	
    pin_irq_hdr(3);
    /* leave interrupt */
    os_irq_unlock(level);
}
void EXTI4_IRQHandler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();	
    pin_irq_hdr(4);
    /* leave interrupt */
    os_irq_unlock(level);
}
void EXTI9_5_IRQHandler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();	
    if (EXTI_GetITStatus(EXTI_Line5) != RESET)
    {
        pin_irq_hdr(5);
    }
    if (EXTI_GetITStatus(EXTI_Line6) != RESET)
    {
        pin_irq_hdr(6);
    }
    if (EXTI_GetITStatus(EXTI_Line7) != RESET)
    {
        pin_irq_hdr(7);
    }
    if (EXTI_GetITStatus(EXTI_Line8) != RESET)
    {
        pin_irq_hdr(8);
    }
    if (EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        pin_irq_hdr(9);
    }
    /* leave interrupt */
    os_irq_unlock(level);
}
void EXTI15_10_IRQHandler(void)
{
    os_base_t level;
    /* enter interrupt */
    level = os_irq_lock();	
    if (EXTI_GetITStatus(EXTI_Line10) != RESET)
    {
        pin_irq_hdr(10);
    }
    if (EXTI_GetITStatus(EXTI_Line11) != RESET)
    {
        pin_irq_hdr(11);
    }
    if (EXTI_GetITStatus(EXTI_Line12) != RESET)
    {
        pin_irq_hdr(12);
    }
    if (EXTI_GetITStatus(EXTI_Line13) != RESET)
    {
        pin_irq_hdr(13);
    }
    if (EXTI_GetITStatus(EXTI_Line14) != RESET)
    {
        pin_irq_hdr(14);
    }
    if (EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
        pin_irq_hdr(15);
    }
    /* leave interrupt */
    os_irq_unlock(level);
}


/**
 ***********************************************************************************************************************
 * @brief           os_hw_pin_init:enable gpio clk,register pin device.
 *
 * @param[in]       none
 *
 * @return          Return init result.
 * @retval          OS_EOK       init success.
 * @retval          Others       init failed.
 ***********************************************************************************************************************
 */
int os_hw_pin_init(void)
{
    /*enable clock*/
    RCC_APB2PeriphClockCmd(RCC_AHBENR_GPIOA | RCC_AHBENR_GPIOB, ENABLE); 

//    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF_15);                      //Disable JTDI   AF to  AF15
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_15);                       //Disable JTDO/TRACESWO   AF to  AF15
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_15);                       //Disable NJRST   AF to  AF15
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_15);                       //Disable AF Funtion   AF to  AF15
    /*disable jtag only swd mode can be used*/
//    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);		

    return os_device_pin_register(0, &_MM32_pin_ops, OS_NULL);
}

#endif /* OS_USING_PIN */
