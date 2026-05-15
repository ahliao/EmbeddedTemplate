#include "platform_lptim.h"

#include "stm32u5xx_hal.h"

#include <string.h>

#define LPTIM1_PRESCALER_DIVISOR 128U
#define LPTIM1_TICK_HZ (LSI_VALUE / LPTIM1_PRESCALER_DIVISOR)

struct platform_lptim {
    LPTIM_HandleTypeDef handle;
    platform_lptim_config_t config;
    platform_lptim_callbacks_t callbacks;
    uint8_t initialized;
};

static platform_lptim_t periodic_lptim;

static platform_status_t map_hal_status(HAL_StatusTypeDef status)
{
    switch (status) {
    case HAL_OK:
        return PLATFORM_OK;
    case HAL_BUSY:
        return PLATFORM_BUSY;
    case HAL_TIMEOUT:
        return PLATFORM_TIMEOUT;
    case HAL_ERROR:
    default:
        return PLATFORM_ERROR;
    }
}

static platform_lptim_t *context_from_handle(LPTIM_HandleTypeDef *handle)
{
    if (handle == &periodic_lptim.handle) {
        return &periodic_lptim;
    }

    return NULL;
}

static platform_status_t period_to_autoreload(uint32_t period_ms, uint32_t *autoreload)
{
    if ((period_ms == 0U) || (autoreload == NULL) || (LPTIM1_TICK_HZ == 0U)) {
        return PLATFORM_INVALID_ARG;
    }

    const uint64_t ticks = (((uint64_t)LPTIM1_TICK_HZ * period_ms) + 999ULL) / 1000ULL;
    if ((ticks == 0ULL) || (ticks > 0x10000ULL)) {
        return PLATFORM_INVALID_ARG;
    }

    *autoreload = (uint32_t)(ticks - 1ULL);
    return PLATFORM_OK;
}

platform_lptim_t *platform_lptim_get(platform_lptim_id_t id)
{
    switch (id) {
    case PLATFORM_LPTIM_ID_PERIODIC:
        return &periodic_lptim;
    case PLATFORM_LPTIM_ID_COUNT:
    default:
        return NULL;
    }
}

platform_status_t platform_lptim_init(platform_lptim_t *timer, const platform_lptim_config_t *config)
{
    if ((timer == NULL) || (config == NULL)) {
        return PLATFORM_INVALID_ARG;
    }

    if (timer != &periodic_lptim) {
        return PLATFORM_INVALID_ARG;
    }

    uint32_t autoreload = 0U;
    platform_status_t status = period_to_autoreload(config->period_ms, &autoreload);
    if (status != PLATFORM_OK) {
        return status;
    }

    memset(&timer->handle, 0, sizeof(timer->handle));

    timer->handle.Instance = LPTIM1;
    timer->handle.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    timer->handle.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV128;
    timer->handle.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
    timer->handle.Init.Period = autoreload;
    timer->handle.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
    timer->handle.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
    timer->handle.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
    timer->handle.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
    timer->handle.Init.RepetitionCounter = 0U;

    status = map_hal_status(HAL_LPTIM_Init(&timer->handle));
    if (status != PLATFORM_OK) {
        return status;
    }

    timer->config = *config;
    timer->initialized = 1U;

    return PLATFORM_OK;
}

platform_status_t platform_lptim_deinit(platform_lptim_t *timer)
{
    if (timer == NULL) {
        return PLATFORM_INVALID_ARG;
    }

    timer->initialized = 0U;
    return map_hal_status(HAL_LPTIM_DeInit(&timer->handle));
}

platform_status_t platform_lptim_start(platform_lptim_t *timer)
{
    if ((timer == NULL) || (timer->initialized == 0U)) {
        return PLATFORM_INVALID_ARG;
    }

    return map_hal_status(HAL_LPTIM_Counter_Start_IT(&timer->handle));
}

platform_status_t platform_lptim_stop(platform_lptim_t *timer)
{
    if ((timer == NULL) || (timer->initialized == 0U)) {
        return PLATFORM_INVALID_ARG;
    }

    return map_hal_status(HAL_LPTIM_Counter_Stop_IT(&timer->handle));
}

platform_status_t platform_lptim_register_callbacks(platform_lptim_t *timer,
                                                    const platform_lptim_callbacks_t *callbacks)
{
    if (timer == NULL) {
        return PLATFORM_INVALID_ARG;
    }

    if (callbacks == NULL) {
        memset(&timer->callbacks, 0, sizeof(timer->callbacks));
    } else {
        timer->callbacks = *callbacks;
    }

    return PLATFORM_OK;
}

void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef *lptim_handle)
{
    if (lptim_handle->Instance == LPTIM1) {
        __HAL_RCC_LSI_ENABLE();
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == 0U) {
        }

        __HAL_RCC_LPTIM1_CONFIG(RCC_LPTIM1CLKSOURCE_LSI);
        __HAL_RCC_LPTIM1_CLK_ENABLE();

        HAL_NVIC_SetPriority(LPTIM1_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
    }
}

void HAL_LPTIM_MspDeInit(LPTIM_HandleTypeDef *lptim_handle)
{
    if (lptim_handle->Instance == LPTIM1) {
        HAL_NVIC_DisableIRQ(LPTIM1_IRQn);
        __HAL_RCC_LPTIM1_CLK_DISABLE();
    }
}

void LPTIM1_IRQHandler(void)
{
    HAL_LPTIM_IRQHandler(&periodic_lptim.handle);
}

void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *lptim_handle)
{
    platform_lptim_t *timer = context_from_handle(lptim_handle);

    if ((timer != NULL) && (timer->callbacks.period_elapsed != NULL)) {
        timer->callbacks.period_elapsed(timer, timer->callbacks.user_context);
    }
}
