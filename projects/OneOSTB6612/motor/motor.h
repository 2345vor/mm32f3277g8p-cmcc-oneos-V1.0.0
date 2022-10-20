/*********************************************************************************************************************
* COPYRIGHT NOTICE
* Copyright (c) 2019,逐飞科技
* All rights reserved.
* 技术讨论QQ群：一群：179029047(已满)  二群：244861897
*
* 以下所有内容版权均属逐飞科技所有，未经允许不得用于商业用途，
* 欢迎各位使用并传播本程序，修改内容时必须保留逐飞科技的版权声明。
*
* @file				pwm
* @company			成都逐飞科技有限公司
* @author			逐飞科技(QQ3184284598)
* @version			查看doc内version文件 版本说明
* @Software			IAR 8.3 or MDK 5.28
* @Target core		MM32F3277
* @Taobao			https://seekfree.taobao.com/
* @date				2021-02-22
********************************************************************************************************************/

#ifndef _motor_h
#define _motor_h

//#include "common.h"
//#include "hal_tim.h"
#include <board.h>
#include "timer.h"
#include "drv_common.h"
#include <os_task.h>
#include <device.h>
#include <string.h>
#include <arch_interrupt.h>
#include <os_task.h>
#include <device.h>
#include <os_memory.h>
#include <os_util.h>
#include <os_assert.h>
#include <drv_cfg.h>


#define DBG_TAG "motor"

#define PWM_DUTY_MAX		50000

//数据类型声明
typedef unsigned char		uint8;													//  8 bits 
typedef unsigned short int	uint16;													// 16 bits 
typedef unsigned long int	uint32;													// 32 bits 
typedef unsigned long long	uint64;													// 64 bits 

typedef char				int8;													//  8 bits 
typedef short int			int16;													// 16 bits 
typedef long  int			int32;													// 32 bits 
typedef long  long			int64;													// 64 bits 

typedef volatile int8		vint8;													//  8 bits 
typedef volatile int16		vint16;													// 16 bits 
typedef volatile int32		vint32;													// 32 bits 
typedef volatile int64		vint64;													// 64 bits 

typedef volatile uint8		vuint8;													//  8 bits 
typedef volatile uint16		vuint16;												// 16 bits 
typedef volatile uint32		vuint32;												// 32 bits 
typedef volatile uint64		vuint64;												// 64 bits 



typedef enum //枚举端口方向
{
	A0 = 0x00,	A1 = 0x01,	A2 = 0x02,	A3 = 0x03,	A4 = 0x04,	A5 = 0x05,	A6 = 0x06,	A7 = 0x07,
	A8 = 0x08,	A9 = 0x09,	A10 = 0x0A,	A11 = 0x0B,	A12 = 0x0C,	A13 = 0x0D,	A14 = 0x0E,	A15 = 0x0F,

	B0 = 0x10,	B1 = 0x11,	B2 = 0x12,	B3 = 0x13,	B4 = 0x14,	B5 = 0x15,	B6 = 0x16,	B7 = 0x17,
	B8 = 0x18,	B9 = 0x19,	B10 = 0x1A,	B11 = 0x1B,	B12 = 0x1C,	B13 = 0x1D,	B14 = 0x1E,	B15 = 0x1F,

	C0 = 0x20,	C1 = 0x21,	C2 = 0x22,	C3 = 0x23,	C4 = 0x24,	C5 = 0x25,	C6 = 0x26,	C7 = 0x27,
	C8 = 0x28,	C9 = 0x29,	C10 = 0x2A,	C11 = 0x2B,	C12 = 0x2C,	C13 = 0x2D,	C14 = 0x2E,	C15 = 0x2F,

	D0 = 0x30,	D1 = 0x31,	D2 = 0x32,	D3 = 0x33,	D4 = 0x34,	D5 = 0x35,	D6 = 0x36,	D7 = 0x37,
	D8 = 0x38,	D9 = 0x39,	D10 = 0x3A,	D11 = 0x3B,	D12 = 0x3C,	D13 = 0x3D,	D14 = 0x3E,	D15 = 0x3F,

	E0 = 0x40,	E1 = 0x41,	E2 = 0x42,	E3 = 0x43,	E4 = 0x44,	E5 = 0x45,	E6 = 0x46,	E7 = 0x47,
	E8 = 0x48,	E9 = 0x49,	E10 = 0x4A,	E11 = 0x4B,	E12 = 0x4C,	E13 = 0x4D,	E14 = 0x4E,	E15 = 0x4F,

	F0 = 0x50,	F1 = 0x51,	F2 = 0x52,	F3 = 0x53,	F4 = 0x54,	F5 = 0x55,	F6 = 0x56,	F7 = 0x57,
	F8 = 0x58,	F9 = 0x59,	F10 = 0x5A,	F11 = 0x5B,	F12 = 0x5C,	F13 = 0x5D,	F14 = 0x5E,	F15 = 0x5F,

	G0 = 0x60,	G1 = 0x61,	G2 = 0x62,	G3 = 0x63,	G4 = 0x64,	G5 = 0x65,	G6 = 0x66,	G7 = 0x67,
	G8 = 0x68,	G9 = 0x69,	G10 = 0x6A,	G11 = 0x6B,	G12 = 0x6C,	G13 = 0x6D,	G14 = 0x6E,	G15 = 0x6F,

	H0 = 0x70,	H1 = 0x71,	H2 = 0x72,	H3 = 0x73,
}PIN_enum;
typedef enum																					// 枚举端口方向
{
	GPI = 0x00,																					// 定义管脚输入
	GPO = 0x03,																					// 定义管脚输出
}GPIODIR_enum;

typedef enum																					// 枚举端口方向
{
	GPI_ANAOG_IN		= 0x00,																	// 定义管脚模拟输入
	GPI_FLOATING_IN		= 0x04,																	// 定义管脚浮空输入
	GPI_PULL_DOWN		= 0x08,																	// 定义管脚下拉输入
	GPI_PULL_UP			= 0x08,																	// 定义管脚上拉输入

	GPO_PUSH_PULL		= 0x00,																	// 定义管脚推挽输出
	GPO_OPEN_DTAIN		= 0x04,																	// 定义管脚开漏输出
	GPO_AF_PUSH_PUL		= 0x08,																	// 定义管脚复用推挽输出
	GPO_AF_OPEN_DTAIN	= 0x0C,																	// 定义管脚复用开漏输出
}GPIOMODE_enum;

typedef enum																					// 枚举端口电平
{
	GPIO_AF0 = 0x00,																			// 引脚复用功能选项  0
	GPIO_AF1 = 0x01,																			// 引脚复用功能选项  1
	GPIO_AF2 = 0x02,																			// 引脚复用功能选项  2
	GPIO_AF3 = 0x03,																			// 引脚复用功能选项  3
	GPIO_AF4 = 0x04,																			// 引脚复用功能选项  4
	GPIO_AF5 = 0x05,																			// 引脚复用功能选项  5
	GPIO_AF6 = 0x06,																			// 引脚复用功能选项  6
	GPIO_AF7 = 0x07,																			// 引脚复用功能选项  7
	GPIO_AF8 = 0x08,																			// 引脚复用功能选项  8
	GPIO_AF9 = 0x09,																			// 引脚复用功能选项  9
	GPIO_AF10 = 0x0A,																			// 引脚复用功能选项 10
	GPIO_AF11 = 0x0B,																			// 引脚复用功能选项 11
	GPIO_AF12 = 0x0C,																			// 引脚复用功能选项 12
	GPIO_AF13 = 0x0D,																			// 引脚复用功能选项 13
	GPIO_AF14 = 0x0E,																			// 引脚复用功能选项 14
	GPIO_AF15 = 0x0F,																			// 引脚复用功能选项 15
}GPIOAF_enum;

typedef enum
{
	// Advanced Timer 16 bits
	TIM_1					= 0x1000,												// 0x1000[APB2]	0x0000[APB_EN BIT] 0x0000[TIM index]
	TIM_8					= 0x1011,												// 0x1000[APB2]	0x0010[APB_EN BIT] 0x0001[TIM index]

	// General Timer 32 bits
	TIM_2					= 0x0002,												// 0x0000[APB1]	0x0000[APB_EN BIT] 0x0002[TIM index]
	TIM_5					= 0x0033,												// 0x0000[APB1]	0x0030[APB_EN BIT] 0x0003[TIM index]

	// General Timer 16 bits
	TIM_3					= 0x0014,												// 0x0000[APB1]	0x0010[APB_EN BIT] 0x0004[TIM index]
	TIM_4					= 0x0025,												// 0x0000[APB1]	0x0020[APB_EN BIT] 0x0005[TIM index]

	// Basic Timer 16 bits
	TIM_6					= 0x0046,												// 0x0000[APB1]	0x0040[APB_EN BIT] 0x0006[TIM index]
	TIM_7					= 0x0057,												// 0x0000[APB1]	0x0050[APB_EN BIT] 0x0007[TIM index]
}TIM_enum;

typedef enum
{
	// Advanced Timer 16 bits TIM1
	TIM_1_CH1_A08			= 0x1108,												// 0x1000[CH1] 0x0100[AF1] 0x0000[A group] 0x0008[pin  8]
	TIM_1_CH1_E09			= 0x1149,												// 0x1000[CH1] 0x0100[AF1] 0x0040[E group] 0x0009[pin  9]

	TIM_1_CH2_A09			= 0x2109,												// 0x2000[CH2] 0x0100[AF1] 0x0000[A group] 0x0009[pin  9]
	TIM_1_CH2_E11			= 0x214B,												// 0x2000[CH2] 0x0100[AF1] 0x0040[E group] 0x000B[pin 11]

	TIM_1_CH3_A10			= 0x310A,												// 0x3000[CH3] 0x0100[AF1] 0x0000[A group] 0x000A[pin 10]
	TIM_1_CH3_E13			= 0x314D,												// 0x3000[CH2] 0x0100[AF1] 0x0040[E group] 0x000D[pin 13]

	TIM_1_CH4_A11			= 0x410B,												// 0x4000[CH4] 0x0100[AF1] 0x0000[A group] 0x000B[pin 11]
	TIM_1_CH4_E14			= 0x414E,												// 0x4000[CH4] 0x0100[AF1] 0x0040[E group] 0x000E[pin 14]

	// Advanced Timer 16 bits TIM8
	TIM_8_CH1_C06			= 0x1326,												// 0x1000[CH1] 0x0300[AF3] 0x0020[C group] 0x0006[pin  6]

	TIM_8_CH2_C07			= 0x2327,												// 0x2000[CH2] 0x0300[AF3] 0x0020[C group] 0x0007[pin  7]

	TIM_8_CH3_C08			= 0x3328,												// 0x3000[CH3] 0x0300[AF3] 0x0020[C group] 0x0008[pin  8]

	TIM_8_CH4_C09			= 0x4329,												// 0x4000[CH4] 0x0300[AF3] 0x0020[C group] 0x0009[pin  9]

	// General Timer 32 bits TIM2
	TIM_2_CH1_A00			= 0x1100,												// 0x1000[CH1] 0x0100[AF1] 0x0000[A group] 0x0000[pin  0]
	TIM_2_CH1_A05			= 0x1105,												// 0x1000[CH1] 0x0100[AF1] 0x0000[A group] 0x0005[pin  5]
	TIM_2_CH1_A15			= 0x110F,												// 0x1000[CH1] 0x0100[AF1] 0x0000[A group] 0x000F[pin 15]

	TIM_2_CH2_A01			= 0x2101,												// 0x2000[CH2] 0x0100[AF1] 0x0000[A group] 0x0001[pin  1]
	TIM_2_CH2_B03			= 0x2113,												// 0x2000[CH2] 0x0100[AF1] 0x0010[B group] 0x0003[pin  3]

	TIM_2_CH3_A02			= 0x3102,												// 0x3000[CH3] 0x0100[AF1] 0x0000[A group] 0x0002[pin  2]
	TIM_2_CH3_B10			= 0x311A,												// 0x3000[CH3] 0x0100[AF1] 0x0010[B group] 0x000A[pin 10]

	TIM_2_CH4_A03			= 0x4103,												// 0x4000[CH4] 0x0100[AF1] 0x0000[A group] 0x0003[pin  3]
	TIM_2_CH4_B11			= 0x411B,												// 0x4000[CH4] 0x0100[AF1] 0x0010[B group] 0x000B[pin 11]

	// General Timer 32 bits TIM5
	TIM_5_CH1_A00			= 0x1200,												// 0x1000[CH1] 0x0200[AF2] 0x0000[A group] 0x0000[pin  0]

	TIM_5_CH2_A01			= 0x2201,												// 0x2000[CH2] 0x0200[AF2] 0x0000[A group] 0x0001[pin  1]

	TIM_5_CH3_A02			= 0x3202,												// 0x3000[CH3] 0x0200[AF2] 0x0000[A group] 0x0002[pin  2]

	TIM_5_CH4_A03			= 0x4203,												// 0x4000[CH4] 0x0200[AF2] 0x0000[A group] 0x0003[pin  3]

	// General Timer 16 bits TIM3
	TIM_3_CH1_A06			= 0x1206,												// 0x1000[CH1] 0x0200[AF2] 0x0000[A group] 0x0006[pin  6]
	TIM_3_CH1_B04			= 0x1214,												// 0x1000[CH1] 0x0200[AF2] 0x0010[B group] 0x0004[pin  4]
	TIM_3_CH1_C06			= 0x1226,												// 0x1000[CH1] 0x0200[AF2] 0x0020[C group] 0x0006[pin  6]

	TIM_3_CH2_A07			= 0x2207,												// 0x2000[CH2] 0x0200[AF2] 0x0000[A group] 0x0007[pin  7]
	TIM_3_CH2_B05			= 0x2215,												// 0x2000[CH2] 0x0200[AF2] 0x0010[B group] 0x0005[pin  5]
	TIM_3_CH2_C07			= 0x2227,												// 0x2000[CH2] 0x0200[AF2] 0x0020[C group] 0x0007[pin  7]

	TIM_3_CH3_B00			= 0x3210,												// 0x3000[CH3] 0x0200[AF2] 0x0010[B group] 0x0000[pin  0]
	TIM_3_CH3_C08			= 0x3228,												// 0x3000[CH3] 0x0200[AF2] 0x0020[C group] 0x0008[pin  8]

	TIM_3_CH4_B01			= 0x4211,												// 0x4000[CH4] 0x0200[AF2] 0x0010[B group] 0x0001[pin  1]
	TIM_3_CH4_C09			= 0x4229,												// 0x4000[CH4] 0x0200[AF2] 0x0020[C group] 0x0009[pin  9]

	// General Timer 16 bits TIM4
	TIM_4_CH1_B06			= 0x1216,												// 0x1000[CH1] 0x0200[AF2] 0x0010[B group] 0x0006[pin  6]
	TIM_4_CH1_D12			= 0x123C,												// 0x1000[CH1] 0x0200[AF2] 0x0030[D group] 0x000C[pin 12]

	TIM_4_CH2_B07			= 0x2217,												// 0x2000[CH2] 0x0200[AF2] 0x0010[B group] 0x0007[pin  7]
	TIM_4_CH2_D13			= 0x223D,												// 0x2000[CH2] 0x0200[AF2] 0x0030[D group] 0x000D[pin 13]

	TIM_4_CH3_B08			= 0x3218,												// 0x3000[CH3] 0x0200[AF2] 0x0010[B group] 0x0008[pin  8]
	TIM_4_CH3_D14			= 0x323E,												// 0x3000[CH3] 0x0200[AF2] 0x0030[D group] 0x000E[pin 14]

	TIM_4_CH4_B09			= 0x4219,												// 0x4000[CH4] 0x0200[AF2] 0x0010[B group] 0x0009[pin  9]
	TIM_4_CH4_D15			= 0x423F,												// 0x4000[CH4] 0x0200[AF2] 0x0030[D group] 0x000F[pin 15]
}TIM_PWMPIN_enum;

void afio_init (PIN_enum pin, GPIODIR_enum dir, GPIOAF_enum af, GPIOMODE_enum mode);
void pwm_init (TIM_enum tim, TIM_PWMPIN_enum pin, uint32 freq, uint32 duty);
void pwm_enable (TIM_enum tim);
void pwm_disable (TIM_enum tim);
void pwm_duty_updata (TIM_enum tim, TIM_PWMPIN_enum pin, uint32 duty);

#endif