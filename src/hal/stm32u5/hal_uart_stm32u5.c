#include "hal_uart_stm32u5.h"

#include <string.h>

static hal_uart_stm32u5_t *to_stm32u5(hal_uart_t *uart)
{
    return (hal_uart_stm32u5_t *)uart;
}

static hal_if_status_t map_hal_status(HAL_StatusTypeDef status)
{
    switch (status) {
    case HAL_OK:
        return HAL_IF_OK;
    case HAL_BUSY:
        return HAL_IF_BUSY;
    case HAL_TIMEOUT:
        return HAL_IF_TIMEOUT;
    case HAL_ERROR:
    default:
        return HAL_IF_ERROR;
    }
}

static uint32_t map_data_bits(hal_uart_data_bits_t data_bits)
{
    return (data_bits == HAL_IF_UART_DATA_BITS_9) ? LL_USART_DATAWIDTH_9B : LL_USART_DATAWIDTH_8B;
}

static uint32_t map_parity(hal_uart_parity_t parity)
{
    switch (parity) {
    case HAL_IF_UART_PARITY_EVEN:
        return LL_USART_PARITY_EVEN;
    case HAL_IF_UART_PARITY_ODD:
        return LL_USART_PARITY_ODD;
    case HAL_IF_UART_PARITY_NONE:
    default:
        return LL_USART_PARITY_NONE;
    }
}

static uint32_t map_stop_bits(hal_uart_stop_bits_t stop_bits)
{
    return (stop_bits == HAL_IF_UART_STOP_BITS_2) ? LL_USART_STOPBITS_2 : LL_USART_STOPBITS_1;
}

static uint32_t map_flow_control(hal_uart_flow_control_t flow_control)
{
    return (flow_control == HAL_IF_UART_FLOW_CONTROL_RTS_CTS) ? LL_USART_HWCONTROL_RTS_CTS : LL_USART_HWCONTROL_NONE;
}

static int timed_out(uint32_t start_ms, uint32_t timeout_ms)
{
    if (timeout_ms == HAL_IF_WAIT_FOREVER) {
        return 0;
    }

    return (HAL_GetTick() - start_ms) >= timeout_ms;
}

static hal_if_status_t wait_for_tx_ready(USART_TypeDef *instance, uint32_t timeout_ms)
{
    const uint32_t start_ms = HAL_GetTick();

    while (!LL_USART_IsActiveFlag_TXE_TXFNF(instance)) {
        if (timed_out(start_ms, timeout_ms)) {
            return HAL_IF_TIMEOUT;
        }
    }

    return HAL_IF_OK;
}

static hal_if_status_t wait_for_rx_ready(USART_TypeDef *instance, uint32_t timeout_ms)
{
    const uint32_t start_ms = HAL_GetTick();

    while (!LL_USART_IsActiveFlag_RXNE_RXFNE(instance)) {
        if (LL_USART_IsActiveFlag_PE(instance) || LL_USART_IsActiveFlag_FE(instance) ||
            LL_USART_IsActiveFlag_NE(instance) || LL_USART_IsActiveFlag_ORE(instance)) {
            return HAL_IF_ERROR;
        }

        if (timed_out(start_ms, timeout_ms)) {
            return HAL_IF_TIMEOUT;
        }
    }

    return HAL_IF_OK;
}

static hal_if_status_t wait_for_tx_complete(USART_TypeDef *instance, uint32_t timeout_ms)
{
    const uint32_t start_ms = HAL_GetTick();

    while (!LL_USART_IsActiveFlag_TC(instance)) {
        if (timed_out(start_ms, timeout_ms)) {
            return HAL_IF_TIMEOUT;
        }
    }

    return HAL_IF_OK;
}

static void clear_error_flags(USART_TypeDef *instance)
{
    LL_USART_ClearFlag_PE(instance);
    LL_USART_ClearFlag_FE(instance);
    LL_USART_ClearFlag_NE(instance);
    LL_USART_ClearFlag_ORE(instance);
}

hal_if_status_t hal_uart_stm32u5_bind(hal_uart_stm32u5_t *context,
                                      USART_TypeDef *instance,
                                      UART_HandleTypeDef *hal_handle)
{
    if ((context == NULL) || (instance == NULL)) {
        return HAL_IF_INVALID_ARG;
    }

    memset(context, 0, sizeof(*context));
    context->instance = instance;
    context->hal_handle = hal_handle;
    memcpy(&context->hal_handle, hal_handle, sizeof(*hal_handle));

    return HAL_IF_OK;
}

hal_uart_t *hal_uart_stm32u5_as_handle(hal_uart_stm32u5_t *context)
{
    return (hal_uart_t *)context;
}

hal_if_status_t hal_uart_init(hal_uart_t *uart, const hal_uart_config_t *config)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->instance == NULL) || (config == NULL) ||
        (config->baudrate == 0U) || (config->source_clock_hz == 0U)) {
        return HAL_IF_INVALID_ARG;
    }

    LL_USART_Disable(context->instance);
    LL_USART_SetPrescaler(context->instance, LL_USART_PRESCALER_DIV1);
    LL_USART_SetDataWidth(context->instance, map_data_bits(config->data_bits));
    LL_USART_SetStopBitsLength(context->instance, map_stop_bits(config->stop_bits));
    LL_USART_SetParity(context->instance, map_parity(config->parity));
    LL_USART_SetTransferDirection(context->instance, LL_USART_DIRECTION_TX_RX);
    LL_USART_SetHWFlowCtrl(context->instance, map_flow_control(config->flow_control));
    LL_USART_SetOverSampling(context->instance, LL_USART_OVERSAMPLING_16);
    LL_USART_SetBaudRate(context->instance,
                         config->source_clock_hz,
                         LL_USART_PRESCALER_DIV1,
                         LL_USART_OVERSAMPLING_16,
                         config->baudrate);
    clear_error_flags(context->instance);
    LL_USART_Enable(context->instance);
    context->config = *config;

    return HAL_IF_OK;
}

hal_if_status_t hal_uart_deinit(hal_uart_t *uart)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->instance == NULL)) {
        return HAL_IF_INVALID_ARG;
    }

    LL_USART_Disable(context->instance);
    LL_USART_SetTransferDirection(context->instance, 0U);
    LL_USART_SetHWFlowCtrl(context->instance, LL_USART_HWCONTROL_NONE);

    return HAL_IF_OK;
}

hal_if_status_t hal_uart_transmit(hal_uart_t *uart, const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->instance == NULL) || ((data == NULL) && (length > 0U))) {
        return HAL_IF_INVALID_ARG;
    }

    if (length == 0U) {
        return HAL_IF_OK;
    }

    for (size_t i = 0; i < length; ++i) {
        hal_if_status_t status = wait_for_tx_ready(context->instance, timeout_ms);
        if (status != HAL_IF_OK) {
            return status;
        }

        LL_USART_TransmitData8(context->instance, data[i]);
    }

    return wait_for_tx_complete(context->instance, timeout_ms);
}

hal_if_status_t hal_uart_receive(hal_uart_t *uart, uint8_t *data, size_t length, uint32_t timeout_ms)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->instance == NULL) || ((data == NULL) && (length > 0U))) {
        return HAL_IF_INVALID_ARG;
    }

    for (size_t i = 0; i < length; ++i) {
        hal_if_status_t status = wait_for_rx_ready(context->instance, timeout_ms);
        if (status != HAL_IF_OK) {
            if (status == HAL_IF_ERROR) {
                clear_error_flags(context->instance);
            }
            return status;
        }

        data[i] = LL_USART_ReceiveData8(context->instance);
    }

    return HAL_IF_OK;
}

hal_if_status_t hal_uart_transmit_dma(hal_uart_t *uart, const uint8_t *data, size_t length)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->hal_handle == NULL) || ((data == NULL) && (length > 0U)) ||
        (length > UINT16_MAX)) {
        return HAL_IF_INVALID_ARG;
    }

    if (length == 0U) {
        return HAL_IF_OK;
    }

    return map_hal_status(HAL_UART_Transmit_DMA(context->hal_handle, data, (uint16_t)length));
}

hal_if_status_t hal_uart_receive_dma(hal_uart_t *uart, uint8_t *data, size_t length)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->hal_handle == NULL) || ((data == NULL) && (length > 0U)) ||
        (length > UINT16_MAX)) {
        return HAL_IF_INVALID_ARG;
    }

    if (length == 0U) {
        return HAL_IF_OK;
    }

    context->dma_rx_length = length;
    return map_hal_status(HAL_UART_Receive_DMA(context->hal_handle, data, (uint16_t)length));
}

hal_if_status_t hal_uart_abort(hal_uart_t *uart)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->hal_handle == NULL)) {
        return HAL_IF_INVALID_ARG;
    }

    return map_hal_status(HAL_UART_Abort(context->hal_handle));
}

hal_if_status_t hal_uart_abort_transmit(hal_uart_t *uart)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->hal_handle == NULL)) {
        return HAL_IF_INVALID_ARG;
    }

    return map_hal_status(HAL_UART_AbortTransmit(context->hal_handle));
}

hal_if_status_t hal_uart_abort_receive(hal_uart_t *uart)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if ((context == NULL) || (context->hal_handle == NULL)) {
        return HAL_IF_INVALID_ARG;
    }

    return map_hal_status(HAL_UART_AbortReceive(context->hal_handle));
}

hal_if_status_t hal_uart_register_callbacks(hal_uart_t *uart, const hal_uart_callbacks_t *callbacks)
{
    hal_uart_stm32u5_t *context = to_stm32u5(uart);

    if (context == NULL) {
        return HAL_IF_INVALID_ARG;
    }

    if (callbacks == NULL) {
        memset(&context->callbacks, 0, sizeof(context->callbacks));
    } else {
        context->callbacks = *callbacks;
    }

    return HAL_IF_OK;
}

void hal_uart_stm32u5_on_tx_complete(hal_uart_stm32u5_t *context)
{
    if ((context != NULL) && (context->callbacks.tx_complete != NULL)) {
        context->callbacks.tx_complete(hal_uart_stm32u5_as_handle(context), context->callbacks.user_context);
    }
}

void hal_uart_stm32u5_on_rx_complete(hal_uart_stm32u5_t *context, size_t bytes_received)
{
    if ((context != NULL) && (context->callbacks.rx_complete != NULL)) {
        context->callbacks.rx_complete(hal_uart_stm32u5_as_handle(context),
                                       context->callbacks.user_context,
                                       bytes_received);
    }
}

void hal_uart_stm32u5_on_error(hal_uart_stm32u5_t *context, hal_if_status_t error)
{
    if ((context != NULL) && (context->callbacks.error != NULL)) {
        context->callbacks.error(hal_uart_stm32u5_as_handle(context), context->callbacks.user_context, error);
    }
}
