#include "drv_usart.h"

#ifdef BSP_USING_UART1
struct mm32_uart_info uart1_info = {.huart = UART1, .uart_clk = RCC_APB2ENR_UART1,
                                        .baud_rate = 115200U, .irq = UART1_IRQn,
                                        .tx_pin = GPIO_Pin_9, .tx_pin_source = GPIO_PinSource9,
                                        .rx_pin = GPIO_Pin_10, .rx_pin_source = GPIO_PinSource10,
                                        .pin_port = GPIOA, .pin_clk = RCC_AHBENR_GPIOA,
                                        .gpio_af_idx = GPIO_AF_7, .uart_rcc_func = RCC_APB2PeriphClockCmd,
                                    };
OS_HAL_DEVICE_DEFINE("UART_HandleTypeDef", "uart1", uart1_info);
#endif
                                    
#ifdef BSP_USING_UART6
struct mm32_uart_info uart6_info = {.huart = UART6, .uart_clk = RCC_APB2ENR_UART6,
                                        .baud_rate = 115200U, .irq = UART6_IRQn,
                                        .tx_pin = GPIO_Pin_0, .tx_pin_source = GPIO_PinSource0,
                                        .rx_pin = GPIO_Pin_1, .rx_pin_source = GPIO_PinSource1,
                                        .pin_port = GPIOB, .pin_clk = RCC_AHBENR_GPIOB,
                                        .gpio_af_idx = GPIO_AF_8, .uart_rcc_func = RCC_APB2PeriphClockCmd,
                                    };
OS_HAL_DEVICE_DEFINE("UART_HandleTypeDef", "uart6", uart6_info);
#endif
                                    
                                    
                                    
