#include "FreeRTOS.h"
#include "stm32u5xx_hal.h"
#include "task.h"

extern __IO uint32_t uwTick;

static volatile uint8_t scheduler_started = 0U;

void hal_freertos_timebase_scheduler_started(void)
{
    scheduler_started = 1U;
}

static uint8_t IsInterruptContext(void)
{
    return ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0U) ? 1U : 0U;
}

uint32_t HAL_GetTick(void)
{
    if (scheduler_started == 0U) {
        return uwTick;
    }

    if (IsInterruptContext() != 0U) {
        return (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return uwTick;
    }

    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void HAL_Delay(uint32_t delay_ms)
{
    if ((scheduler_started != 0U) && (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)) {
        if (IsInterruptContext() != 0U) {
            return;
        }

        if (delay_ms == 0U) {
            taskYIELD();
        } else {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }

        return;
    }

    uint32_t tickstart = HAL_GetTick();
    uint32_t wait = delay_ms;

    if (wait < HAL_MAX_DELAY) {
        wait += (uint32_t)HAL_GetTickFreq();
    }

    while ((HAL_GetTick() - tickstart) < wait) {
    }
}
