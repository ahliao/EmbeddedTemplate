#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_gpio.h"
#include "hal_uart_interface.h"
// #include "stm32u5a5_hal_conf.h"

int main(void) {
    HAL_Init();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &gpio);

    while(1) {
        // Main loop of the application
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
        HAL_Delay(1000);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
        HAL_Delay(1000);
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}