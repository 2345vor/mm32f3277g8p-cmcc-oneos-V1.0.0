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

