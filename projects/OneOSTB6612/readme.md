# 1. 介绍🎏
TB6612FNG是一种简单且经济实惠的电机控制方式。TB6612FNG 能够以高达 1.2A 的恒定电流驱动两个电机。在 IC 内部，您会在一个芯片上找到两个标准 H 桥，让您不仅可以控制电机的方向和速度，还可以停止和制动。本指南将详细介绍如何使用 TB6612FNG 电机驱动板。
![在这里插入图片描述](https://img-blog.csdnimg.cn/e72dea23ce064558813e96adfb21a28a.png)
## 1.1 方块图🎏
![在这里插入图片描述](https://img-blog.csdnimg.cn/d69cc4a675694e35b23ce349eab378d0.png)
## 1.2 引脚功能🎏
![在这里插入图片描述](https://img-blog.csdnimg.cn/9b29060641ff44bb80c091463791f674.png)
## 1.3 目标特性🎏
![在这里插入图片描述](https://img-blog.csdnimg.cn/b721184e036a463c82d671963b4ea385.png)
## 1.4 典型应用图🎏
![在这里插入图片描述](https://img-blog.csdnimg.cn/0a8cd192184b4d0ea1dd2277ef931e85.png)

# 2. 所需材料🎏🎏
代码地址：[https://gitee.com/vor2345/00_Customer_Sample_Project](https://gitee.com/vor2345/00_Customer_Sample_Project "https://gitee.com/vor2345/00_Customer_Sample_Project")

要跟随本教程中的电机驱动器示例，以下是您需要的基本组件
1. 一块OneOS 万耦启物开发板🎏
![在这里插入图片描述](https://img-blog.csdnimg.cn/cd7e078d4a71422db22ce7e50727b3fc.png)
2. 调试下载接口：SWD调试接口、JTAG调试接口或者STLink🎏🎏

![在这里插入图片描述](https://img-blog.csdnimg.cn/2e33ceb2ba444d789f80c612b72e924f.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/78052e98d4fd4c42962d3fbf8df1e597.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/bc9365bdef3340f29a5e722c06f2988b.png)

3.  9根母母线，4根公母线🎏🎏🎏

4. 两个TT马达，振动马达，[推荐一个店铺（马达零配件捡漏）： 杰盛电机商行](https://szjs88998899.taobao.com/?spm=a1z10.1-c-s.0.0.230766574n3H6U)

![在这里插入图片描述](https://img-blog.csdnimg.cn/968c82ac524448a48fec6bb587f93270.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/56fc6bd8d2354a88841dd1915eca6296.png)
[电力](https://learn.sparkfun.com/tutorials/electric-power)
电力概述，能量传输率。我们将讨论功率、瓦特、方程式和额定功率的定义。1.21 吉瓦的教程乐趣！
[极性](https://learn.sparkfun.com/tutorials/polarity)
介绍电子元件中的极性。了解什么是极性，哪些部分有极性，以及如何识别它。
[交流电 (AC) 与直流电 (DC)](https://learn.sparkfun.com/tutorials/alternating-current-ac-vs-direct-current-dc)
了解交流和直流的区别、历史、产生交流和直流的不同方式以及应用示例。
[电机和选择合适的电机](https://learn.sparkfun.com/tutorials/motors-and-selecting-the-right-one)
了解所有关于不同类型的电机及其运行方式的信息。

# 3. 选择合适的电机驱动器🎏🎏🎏
在开始之前，让我们先谈谈如何找到适合您需求的电机驱动器。
- 第一步是弄清楚您使用的是什么类型的电机并研究它们的规格。为不够强大的电机选择电机驱动器是没有帮助的。此外，请记住有不同的电机类型（步进、直流、无刷），因此请确保您正在寻找正确类型的电机驱动器。您需要指定您的电机驱动器并确保其电流和电压范围与您的电机兼容。首先，您需要确保您的电机驱动器能够处理电机的额定电压。虽然您通常可以运行略高于额定值的电机，但它往往会缩短电机的使用寿命。

- 电流消耗是第二个因素。您的电机驱动器需要能够驱动与电机拉动一样多的电流。作为一般规则，直接查看电机的停转电流编号（当您保持电机静止时电流消耗存在）。电机在停转时将拉动最大电流。即使您不打算在项目中停止电机，这也是一个安全的数字。如果您的电机驱动器无法处理那么多电流，那么是时候寻找新的电机驱动器（或电机）了。您可能还会注意到电机驱动器通常列出最大连续电流和最大峰值电流。这些规格值得注意，具体取决于您的应用以及电机将承受的压力。
- 本指南涵盖了 TB6612FNG 电机驱动器，其电源范围为2.5V 至 13.5V，能够提供1.2A 连续电流和3.2A 峰值电流（每通道），因此它适用于我们的大多数直流电机。如果 TB6612FNG 不符合您项目的规格，请查看我们的其他各种电机驱动板。

与任何电路板一样，还有其他一些事情需要考虑，例如逻辑电压（基本上是它用来与微控制器通信的电压）和散热。虽然这些事情肯定需要考虑，但它们相对容易使用电平转换器和散热器等东西进行修复。但是，如果您的电机试图拉出的电流超出了驱动器的承受能力，那么您就无能为力了。

# 4. TB6612驱动原理🎏🎏🎏🎏
让我们讨论一下 TB6612FNG 分接头的引脚排列。我们基本上有三种类型的引脚：电源、输入和输出，它们都标在板的背面。
## 4.1 引脚定义🎏🎏🎏🎏
![在这里插入图片描述](https://img-blog.csdnimg.cn/559635b261b94329b845fcf896d16a9e.png)
|引脚标签|	功能|	电源/输入/输出|笔记|
|---- |---- |---- |---- |
|VM	|电机电压	|力量	|这是您为电机供电的地方（2.2V 至 13.5V）|
|VCC	|逻辑电压	|力量	|这是为芯片供电并与微控制器通信的电压（2.7V 至 5.5V）|
|GND	|地面	|力量	|电机电压和逻辑电压的公共接地（所有 GND 引脚均已连接）|
|STBY	|支持	|输入	|允许 H 桥在高电平时工作（具有下拉电阻，因此必须主动拉高）|
|AIN1/BIN1	|通道 A/B 的输入 1	|输入|	确定方向的两个输入之一。|
|AIN2/BIN2	|通道 A/B 的输入 2	|输入|	确定方向的两个输入之一。|
|PWMA/PWMB	|通道 A/B 的 PWM 输入	|输入|	控制速度的 PWM 输入|
|A01/B01	|通道 A/B 的输出 1	|输出	|连接电机的两个输出之一|
|A02/B02	|通道 A/B 的输出 2	|输出|	连接电机的两个输出之一|

## 4.2 控制状态表🎏🎏🎏🎏
现在，快速了解如何控制每个通道。当输出设置为高/低时，您的电机将运行。当它们设置为低/高时，电机将以相反的方向运行。在这两种情况下，速度都由 PWM 输入控制。控制状态表如下

|AIN1|	AIN2|STBY	|PWMA	|A1	|模式|
|---- |---- |---- |---- |---- |---- |
|H|H	|H/L|H~L|L|停止|
|L	|H|H	|H~L|H~L	|逆时针|
|L|L	|H/L	|H~L|L	|停止|
|H|L	|H|H~L	|H~L|顺时针|

> 不要忘记 STBY 必须很高才能驱动电机。



下一步是使用您首选的项目平台连接所有内容。我们正在使用万耦启物开发板（mm32)，如果您使用不同的引脚或不同的微控制器，请记住电机驱动器的 PWM 引脚需要是微控制器上的 PWM 引脚。
## 4.3 引脚接线🎏🎏🎏🎏
万耦启物连接TB6612，然后连接左右电机。
|端口A|位置|端口B|位置|
|--- |--- |--- |--- |
|B6 |万耦启物 |PWMA |TB6612|
|B7 |万耦启物 |PWMB |TB6612 |
|A4 |万耦启物 |AIN1|TB6612|
|A5 |万耦启物 |AIN2 |TB6612 |
|A6 |万耦启物 |BIN1|TB6612 |
|A7 |万耦启物|BIN2 |TB6612|
|GND |万耦启物 |GND|TB6612|
|3V3 |万耦启物 |STBY |TB6612 |
|5V |万耦启物 |VM|TB6612 |
|正极 |左电机 |AO1|TB6612|
|负极 |左电机 |AO2 |TB6612 |
|正极 |右电机 |BO1|TB6612 |
|负极 |右电机|BO2 |TB6612|

实物图
![在这里插入图片描述](https://img-blog.csdnimg.cn/8af893f6a84e47f6b36c640ba71a5f78.png)
# 5. 代码实现🎏🎏🎏🎏🎏
添加三个文件就可以实现正转渐变的效果
## 5.1 motor.c🎏🎏🎏🎏🎏
`motor.c`PWM初始化，定义相关操作函数

```cpp
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

#include "motor.h"
//#include "zf_gpio.h"
#include "drv_gpio.h"
#include "bsp.h"
#include "board.h"

GPIO_TypeDef *gpio_group[8] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH};
extern GPIO_TypeDef *gpio_group[8];
TIM_TypeDef *tim_index[8] = {TIM1, TIM8, TIM2, TIM5, TIM3, TIM4, TIM6, TIM7};
extern TIM_TypeDef *tim_index[8];
extern uint32_t SystemCoreClock;
uint32 pwm_enable_state[8];


//-------------------------------------------------------------------------------------------------------------------
// @brief		GPIO初始化
// @param		pin			选择的引脚 (可选择范围由 common.h 内PIN_enum枚举值确定)
// @param		dir			引脚的方向
// @param		af			引脚的功能选择
// @param		mode		引脚的模式
// @return		void
// Sample usage:			afio_init(D5, GPO, GPIO_AF0, GPO_AF_PUSH_PUL);
//-------------------------------------------------------------------------------------------------------------------
void afio_init (PIN_enum pin, GPIODIR_enum dir, GPIOAF_enum af, GPIOMODE_enum mode)
{
	uint32 temp;
	uint8 io_group = ((pin&0xf0)>>4);															// 提取IO分组
	uint8 io_pin = (pin&0x0f);																	// 提取IO引脚下标

	RCC->AHBENR |= RCC_AHBENR_GPIOA << io_group;												// 使能 GPIOx 时钟

	if(io_pin < 0x08)																			// 低8位引脚
	{
		temp = gpio_group[io_group]->AFRL;
		temp &= ~(0x0000000f << (io_pin*4));
		temp |= ((uint32)af << (io_pin*4));
		gpio_group[io_group]->AFRL = temp;

		temp = gpio_group[io_group]->CRL;
		temp &= ~(0x0000000f << (io_pin*4));
		temp |= (((uint32)dir|(uint32)mode) << (io_pin*4));
		gpio_group[io_group]->CRL = temp;
	}
	else																						// 高8位引脚
	{
		temp = gpio_group[io_group]->AFRH;
		temp &= ~(0x0000000f << ((io_pin-8)*4));
		temp |= ((uint32)af << ((io_pin-8)*4));
		gpio_group[io_group]->AFRH = temp;

		temp = gpio_group[io_group]->CRH;
		temp &= ~(0x0000000f << ((io_pin-8)*4));
		temp |= (((uint32)dir|(uint32)mode) << ((io_pin-8)*4));
		gpio_group[io_group]->CRH = temp;
	}
}

//-------------------------------------------------------------------------------------------------------------------
// @brief		PWM 引脚初始化 内部调用
// @param		pin				选择 PWM 引脚
// @return		void			NULL
// Sample usage:				pwm_pin_init(pin);
//-------------------------------------------------------------------------------------------------------------------
static void pwm_pin_init (TIM_PWMPIN_enum pin)
{
	afio_init((PIN_enum)(pin &0xff), GPO, (GPIOAF_enum)((pin &0xf00)>>8), GPO_AF_PUSH_PUL);		// 提取对应IO索引 AF功能编码
}

//-------------------------------------------------------------------------------------------------------------------
// @brief		PWM 初始化
// @param		tim				选择 PWM 使用的 TIM
// @param		pin				选择 PWM 引脚
// @param		freq			设置频率 同个模块只有最后一次设置生效
// @param		duty			设置占空比
// @return		void
// Sample usage:						pwm_init(TIM_1, TIM_1_CH1_A08, 10000, 50000/100*ch1);
//-------------------------------------------------------------------------------------------------------------------
void pwm_init (TIM_enum tim, TIM_PWMPIN_enum pin, uint32 freq, uint32 duty)
{
	if(duty > PWM_DUTY_MAX)	return;																// 占空比写入错误
	uint16 freq_div = ((SystemCoreClock / freq) >> 15);											// 计算预分频
	uint16 period_temp = (SystemCoreClock / freq / (freq_div+1));								// 计算自动重装载值
	uint16 match_temp = (uint16)(period_temp*((float)duty/PWM_DUTY_MAX));						// 计算占空比

	pwm_pin_init(pin);																			// 初始化引脚
	if(tim & 0xf000)
		RCC->APB2ENR |= ((uint32_t)0x00000001 << ((tim&0x0ff0) >> 4));							// 使能时钟
	else
		RCC->APB1ENR |= ((uint32_t)0x00000001 << ((tim&0x0ff0) >> 4));							// 使能时钟

	tim_index[(tim&0x0f)]->ARR = period_temp;													// 装载自动重装载值
	tim_index[(tim&0x0f)]->PSC = freq_div;														// 装载预分频
	tim_index[(tim&0x0f)]->CR1 = TIM_CR1_ARPEN;													// 允许自动重装载值的预装载
	tim_index[(tim&0x0f)]->BDTR = TIM_BDTR_MOEN;												// PWM 输出使能

	switch(pin&0xf000)
	{
		case 0x1000:
			tim_index[(tim&0x0f)]->CCMR1 |=														// OC1M [6:4] 110
					TIM_CCMR1_IC1F_1 | TIM_CCMR1_IC1F_2;										// PWM 模式 1
			tim_index[(tim&0x0f)]->CCMR1 |= TIM_CCMR1_OC1PEN;									// 允许输出比较值的预装载
			tim_index[(tim&0x0f)]->CCER |= TIM_CCER_CC1EN;										// 使能通道 1
			tim_index[(tim&0x0f)]->CCR1 = match_temp;											// 装载比较值
			break;
		case 0x2000:
			tim_index[(tim&0x0f)]->CCMR1 |=														// OC1M [6:4] 110
					TIM_CCMR1_IC2F_1 | TIM_CCMR1_IC2F_2;										// PWM 模式 1
			tim_index[(tim&0x0f)]->CCMR1 |= TIM_CCMR1_OC2PEN;									// 允许输出比较值的预装载
			tim_index[(tim&0x0f)]->CCER |= TIM_CCER_CC2EN;										// 使能通道 2
			tim_index[(tim&0x0f)]->CCR2 = match_temp;											// 装载比较值
			break;
		case 0x3000:
			tim_index[(tim&0x0f)]->CCMR2 |=														// OC1M [6:4] 110
					TIM_CCMR2_IC3F_1 | TIM_CCMR2_IC3F_2;										// PWM 模式 1
			tim_index[(tim&0x0f)]->CCMR2 |= TIM_CCMR2_OC3PEN;									// 允许输出比较值的预装载
			tim_index[(tim&0x0f)]->CCER |= TIM_CCER_CC3EN;										// 使能通道 2
			tim_index[(tim&0x0f)]->CCR3 = match_temp;											// 装载比较值
			break;
		case 0x4000:
			tim_index[(tim&0x0f)]->CCMR2 |=														// OC1M [6:4] 110
					TIM_CCMR2_IC4F_1 | TIM_CCMR2_IC4F_2;										// PWM 模式 0
			tim_index[(tim&0x0f)]->CCMR2 |= TIM_CCMR2_OC4PEN;									// 允许输出比较值的预装载
			tim_index[(tim&0x0f)]->CCER |= TIM_CCER_CC4EN;										// 使能通道 2
			tim_index[(tim&0x0f)]->CCR4 = match_temp;											// 装载比较值
			break;
	}
	pwm_enable_state[(tim&0x0f)] = tim_index[(tim&0x0f)]->CCER;

	tim_index[(tim&0x0f)]->CR1 |= TIM_CR1_CEN;													// 使能定时器
}

//-------------------------------------------------------------------------------------------------------------------
// @brief		PWM 使能
// @param		tim				选择 PWM 使用的 TIM
// @return		void
// Sample usage:				pwm_enable(TIM_1);
//-------------------------------------------------------------------------------------------------------------------
void pwm_enable (TIM_enum tim)
{
	tim_index[(tim&0x0f)]->CCER = pwm_enable_state[(tim&0x0f)];
	tim_index[(tim&0x0f)]->CR1 |= TIM_CR1_CEN;													// 使能定时器
}

//-------------------------------------------------------------------------------------------------------------------
// @brief		PWM 失能
// @param		tim				选择 PWM 使用的 TIM
// @return		void
// Sample usage:				pwm_disable(TIM_1);
//-------------------------------------------------------------------------------------------------------------------
void pwm_disable (TIM_enum tim)
{
	pwm_enable_state[(tim&0x0f)] = tim_index[(tim&0x0f)]->CCER;
	tim_index[(tim&0x0f)]->CCER = 0x00000000;
	tim_index[(tim&0x0f)]->CR1 &= ~TIM_CR1_CEN;													// 失能定时器
}

//-------------------------------------------------------------------------------------------------------------------
// @brief		PWM 更新占空比
// @param		tim				选择 PWM 使用的 TIM
// @param		pin				选择 PWM 引脚
// @param		duty			设置占空比
// @return		void
// Sample usage:				pwm_duty_updata(TIM_1, TIM_1_CH1_A08, 500*duty);
//-------------------------------------------------------------------------------------------------------------------
void pwm_duty_updata (TIM_enum tim, TIM_PWMPIN_enum pin, uint32 duty)
{
	if(duty > PWM_DUTY_MAX)	return;																// 占空比写入错误
	uint16 period_temp = tim_index[(tim&0x0f)]->ARR;											// 获取自动重装载值
	uint16 match_temp = (uint16)(period_temp*((float)duty/PWM_DUTY_MAX));						// 计算占空比

	switch(pin&0xf000)																			// 提取通道值
	{
		case 0x1000:
			tim_index[(tim&0x0f)]->CCR1 = match_temp;											// 装载比较值
			break;
		case 0x2000:
			tim_index[(tim&0x0f)]->CCR2 = match_temp;											// 装载比较值
			break;
		case 0x3000:
			tim_index[(tim&0x0f)]->CCR3 = match_temp;											// 装载比较值
			break;
		case 0x4000:
			tim_index[(tim&0x0f)]->CCR4 = match_temp;											// 装载比较值
			break;
	}
}


```
## 5.2 motor.h🎏🎏🎏🎏🎏
`motor.h`，定义相关PWM、TIMER、GPIO端口，配置PWM输出

```cpp
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

```
## 5.3 test_motor.c🎏🎏🎏🎏🎏
 `test_motor.c` ，新建一个线程，执行双电机前进渐变！前进后退函数可以自己调试使用。
 

```cpp
#include <drv_cfg.h>
#include <device.h>
#include <os_clock.h>
#include <os_memory.h>
#include <stdlib.h>
#include <shell.h>
#include <timer/clocksource.h>
#include "motor.h"
#include "drv_gpio.h"

// *************************** 例程说明 ***************************
// 
// 测试需要准备 中国移动 OneOS 启物开发板 一块
// 
// 调试下载需要准备逐飞科技 CMSIS-DAP 调试下载器 或 ARM 调试下载器 一个
// 
// 本例程演示拓展接口上 B6-B7 引脚的 PWM 功能，A4-A7分别负责左右两路方向
// 
// 打开新的工程或者工程移动了位置务必执行以下操作
// 第一步 关闭上面所有打开的文件
// 第二步 project->clean  等待下方进度条走完
// 
// *************************** 例程说明 ***************************

// **************************** 宏定义 ****************************
#define EXPAND_PWM_TIM			TIM_4
#define EXPAND_PWM_FRQE			16000
#define EXPAND_PWM_CH1			TIM_4_CH1_B06
#define EXPAND_PWM_CH2			TIM_4_CH2_B07

// **************************** 宏定义 ****************************

// **************************** 变量定义 ****************************
int pwm_duty = 0;

bool pwm_dir = true;
// **************************** 变量定义 ****************************

// **************************** 代码区域 ****************************
int pwm_motor(void)
{
//	board_init(true);																// 初始化 debug 输出串口
	void turn_forward(int rb,int lb);
	//此处编写用户代码(例如：外设初始化代码等)
	pwm_init(EXPAND_PWM_TIM, EXPAND_PWM_CH1, EXPAND_PWM_FRQE, 0);
	pwm_init(EXPAND_PWM_TIM, EXPAND_PWM_CH2, EXPAND_PWM_FRQE, 0);

	os_pin_mode(A4, PIN_MODE_OUTPUT);
	os_pin_mode(A5, PIN_MODE_OUTPUT);
	os_pin_mode(A6, PIN_MODE_OUTPUT);
	os_pin_mode(A7, PIN_MODE_OUTPUT);
	os_kprintf("\r\nMM32 OneOS EVBoard MISC.");
	os_kprintf("\r\nExample 23_Expand_PWM_Demo.");
	//此处编写用户代码(例如：外设初始化代码等)

	while(1)
	{
		//此处编写需要循环执行的代码
		if(pwm_dir)
			pwm_duty += 50;
		else
			pwm_duty -= 50;

		if(pwm_duty >= PWM_DUTY_MAX)
		{
			pwm_duty = PWM_DUTY_MAX;
			pwm_dir = false;
		}
		if(pwm_duty <= 0)
		{
			pwm_duty = 0;
			pwm_dir = true;
		}
		os_kprintf("pwm_duty: %d; %d \r\n",pwm_duty,pwm_duty/(PWM_DUTY_MAX/100));
		turn_forward(pwm_duty,PWM_DUTY_MAX-pwm_duty);//调用前进程序（此处故意相减）


		os_task_msleep(10);
		//此处编写需要循环执行的代码
	}
}

// *************************** turn_forward函数说明 ***************************
// 功能：前进左右转使能左右轮
// 参数1：rb占空比
// 参数2：lb占空比
// 前进左右分别使能
// 
// *************************** forward ***************************
void turn_forward(int rb,int lb){
	os_pin_write(A4, 0);
	os_pin_write(A5, 1);
	os_pin_write(A6, 0);
	os_pin_write(A7, 1);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH1, rb);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH2, lb);
}

// *************************** forward函数说明 ***************************
// 功能：前进使能左右轮
// 参数1：rll占空比
// 左右使能rll
// 
// *************************** forward ***************************
void forward(int rlf){
	os_pin_write(A4, 0);
	os_pin_write(A5, 1);
	os_pin_write(A6, 0);
	os_pin_write(A7, 1);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH1, rlf);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH2, rlf);
}

// *************************** turn_backward函数说明 ***************************
// 功能：后退左右转使能左右轮
// 参数1：rb占空比
// 参数2：lb占空比
// 后退左右分别使能
// 
// *************************** backward ***************************
void turn_backward(int rb,int lb){
	os_pin_write(A4, 1);
	os_pin_write(A5, 0);
	os_pin_write(A6, 1);
	os_pin_write(A7, 0);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH1, rb);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH2, lb);
}

// *************************** backward函数说明 ***************************
// 功能：后退使能左右轮
// 参数1：rlb占空比
// 左右使能rlb
// 
// *************************** backward ***************************
void backward(int rlb){
	os_pin_write(A4, 1);
	os_pin_write(A5, 0);
	os_pin_write(A6, 1);
	os_pin_write(A7, 0);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH1, rlb);
	pwm_duty_updata(EXPAND_PWM_TIM, EXPAND_PWM_CH2, rlb);
}


// **************************** 代码区域 ****************************
SH_CMD_EXPORT(pwm_motor, pwm_motor, "two pwm motor sample!");


```

我是基于[【例程】OneOS启物开发板板级支持包(BSP)(oo_Customer_Sample_Project)](https://download.csdn.net/download/VOR234/86504837)代码继续编写的，
添加过程如下
![在这里插入图片描述](https://img-blog.csdnimg.cn/c4f227d8adab4aa896bef4e15bfecc64.png)

三个文件添加后效果如下

![在这里插入图片描述](https://img-blog.csdnimg.cn/792ec80808e44f49b1b6c627d9e4a5ca.png)
修改`main.c`，注释打印时间

```cpp
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
 * @file        main.c
 *
 * @brief       User application entry
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <board.h>
#include <timer/clocksource.h>

// *************************** 例程说明 ***************************
// 
// 测试需要准备 <OneOS 启物开发板> 一块
// 推荐 MDK 版本为 MDK5.28a
// 
// 调试下载需要准备 <逐飞科技 CMSIS-DAP 调试下载器> 或 <ARM 调试下载器> 一个
// 或者使用 <标准 Jlink> 进行下载调试连接亦可
// 
// 确认连接无误 上电 可使用调试下载器供电 但推荐使用 USB供电 可通过CH340的Type-C进行供电
// 烧录本例程后 如果使用的是 <逐飞科技 CMSIS-DAP 调试下载器> 或 <ARM 调试下载器>
// 可以直接在串口助手中打开对应串口
// 如果使用的并不是 <逐飞科技 CMSIS-DAP 调试下载器> 或 <ARM 调试下载器>
// 请接上 CH340 标识的Type-C接口到电脑USB 再打开串口调试助手打开对应串口
// 
// 本工程为用户示例工程 仅开启基础功能
// 
// 打开新的工程或者工程移动了位置务必执行以下操作
// 第一步 关闭上面所有打开的文件
// 第二步 project->clean  等待下方进度条走完
// 
// 2021-08-12	SEEKFREE
// 
// *************************** 例程说明 ***************************

static void user_task(void *parameter)
{
	os_int32_t time_count = 1;

	os_task_msleep(1000);
	os_kprintf("\r\nWhat's up?\r\n");
    while (1)
    {
//		os_kprintf("%dh %dm %ds was wasted.\r\n", time_count/3600, time_count/60%60, time_count%60);
		time_count++;
		os_task_msleep(1000);
    }
}

int main(void)
{
    os_task_t *task;

    task = os_task_create("user", user_task, NULL, 1024, 3);
    OS_ASSERT(task);
    os_task_startup(task);

    return 0;
}

```
# 6. 调试验证🎏🎏🎏🎏🎏🎏
## 6.1 编译上传🎏🎏🎏🎏🎏🎏
![在这里插入图片描述](https://img-blog.csdnimg.cn/ee4b8be77485494a84ea87e9689fda3c.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/319cc9d4e85e43d3a2b54e4a212b7a43.png)
## 6.2 实物效果🎏🎏🎏🎏🎏🎏
打开串口，输入`pwm_motor`指令，
小马达疯狂抖动🤣🤣🤣
![请添加图片描述](https://img-blog.csdnimg.cn/d7958b9c178d4502966e44a56bf71e95.gif)
# 7. 总结👻👻👻
有点小遗憾，没有config好加入cube配置😅，但一切走来都是值得的。非常感谢OneOS您们组织这次活动，在这里面我学习到很多单片机实时操作系统开发的知识，期待下一次参加你们的活动！🤣🤣🤣

参考文献：
- [TB6612FNG 连接指南](https://learn.sparkfun.com/tutorials/tb6612fng-hookup-guide/all)
- 	[TB6612FNG 数据表/中文（翻译版）](https://toshiba-semicon-storage.com/info/docget.jsp?did=30692&prodName=TB6612FNG)
