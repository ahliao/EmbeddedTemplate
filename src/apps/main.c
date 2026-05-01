#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_gpio.h"

#include "hal_uart_stm32u5.h"
#include "stm32u5xx_ll_rcc.h"
// #include "stm32u5a5_hal_conf.h"

int main(void) {
    HAL_Init();

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_HSI);

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &gpio);

    UART_HandleTypeDef usart1h;
    hal_uart_stm32u5_t usart1;
    hal_uart_stm32u5_bind(&usart1, USART1, &usart1h);
    hal_uart_config_t usart1cfg = {
        .baudrate = 115200,
        .data_bits = HAL_IF_UART_DATA_BITS_8,
        .parity = HAL_IF_UART_PARITY_NONE,
        .stop_bits = HAL_IF_UART_STOP_BITS_1,
        .flow_control = HAL_IF_UART_FLOW_CONTROL_NONE,
    };
    hal_uart_t *uart = hal_uart_stm32u5_as_handle(&usart1);
    hal_uart_init(uart, &usart1cfg);
    usart1.instance->CR1 = 0x0D;

    char testmsg[] = "TEST";

    while(1) {
        // Main loop of the application
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        hal_uart_transmit(uart, (uint8_t*)testmsg, 4, 100);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_Delay(1000);
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}