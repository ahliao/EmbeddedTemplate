#include "platform_uart.h"

#include "stm32u5xx_hal.h"
#include "stm32u5xx_ll_rcc.h"

#include <string.h>

struct platform_uart {
    UART_HandleTypeDef handle;
    DMA_HandleTypeDef tx_dma;
    DMA_HandleTypeDef rx_dma;
    platform_uart_config_t config;
    platform_uart_callbacks_t callbacks;
    size_t rx_dma_length;
    uint8_t initialized;
};

static platform_uart_t console_uart;

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

static uint32_t map_word_length(platform_uart_data_bits_t data_bits)
{
    return (data_bits == PLATFORM_UART_DATA_BITS_9) ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
}

static uint32_t map_parity(platform_uart_parity_t parity)
{
    switch (parity) {
    case PLATFORM_UART_PARITY_EVEN:
        return UART_PARITY_EVEN;
    case PLATFORM_UART_PARITY_ODD:
        return UART_PARITY_ODD;
    case PLATFORM_UART_PARITY_NONE:
    default:
        return UART_PARITY_NONE;
    }
}

static uint32_t map_stop_bits(platform_uart_stop_bits_t stop_bits)
{
    return (stop_bits == PLATFORM_UART_STOP_BITS_2) ? UART_STOPBITS_2 : UART_STOPBITS_1;
}

static uint32_t map_flow_control(platform_uart_flow_control_t flow_control)
{
    return (flow_control == PLATFORM_UART_FLOW_CONTROL_RTS_CTS) ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
}

static platform_uart_t *context_from_handle(UART_HandleTypeDef *handle)
{
    if (handle == &console_uart.handle) {
        return &console_uart;
    }

    return NULL;
}

static platform_status_t init_console_dma(platform_uart_t *uart)
{
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    uart->tx_dma.Instance = GPDMA1_Channel0;
    uart->tx_dma.Init.Request = GPDMA1_REQUEST_USART1_TX;
    uart->tx_dma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    uart->tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
    uart->tx_dma.Init.SrcInc = DMA_SINC_INCREMENTED;
    uart->tx_dma.Init.DestInc = DMA_DINC_FIXED;
    uart->tx_dma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    uart->tx_dma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    uart->tx_dma.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    uart->tx_dma.Init.SrcBurstLength = 1;
    uart->tx_dma.Init.DestBurstLength = 1;
    uart->tx_dma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT1 | DMA_DEST_ALLOCATED_PORT0;
    uart->tx_dma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    uart->tx_dma.Init.Mode = DMA_NORMAL;

    if (HAL_DMA_Init(&uart->tx_dma) != HAL_OK) {
        return PLATFORM_ERROR;
    }

    uart->rx_dma.Instance = GPDMA1_Channel1;
    uart->rx_dma.Init.Request = GPDMA1_REQUEST_USART1_RX;
    uart->rx_dma.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    uart->rx_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
    uart->rx_dma.Init.SrcInc = DMA_SINC_FIXED;
    uart->rx_dma.Init.DestInc = DMA_DINC_INCREMENTED;
    uart->rx_dma.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    uart->rx_dma.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    uart->rx_dma.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    uart->rx_dma.Init.SrcBurstLength = 1;
    uart->rx_dma.Init.DestBurstLength = 1;
    uart->rx_dma.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
    uart->rx_dma.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    uart->rx_dma.Init.Mode = DMA_NORMAL;

    if (HAL_DMA_Init(&uart->rx_dma) != HAL_OK) {
        return PLATFORM_ERROR;
    }

    __HAL_LINKDMA(&uart->handle, hdmatx, uart->tx_dma);
    __HAL_LINKDMA(&uart->handle, hdmarx, uart->rx_dma);

    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

    return PLATFORM_OK;
}

platform_uart_t *platform_uart_get(platform_uart_id_t id)
{
    switch (id) {
    case PLATFORM_UART_ID_CONSOLE:
        return &console_uart;
    case PLATFORM_UART_ID_COUNT:
    default:
        return NULL;
    }
}

platform_status_t platform_uart_init(platform_uart_t *uart, const platform_uart_config_t *config)
{
    if ((uart == NULL) || (config == NULL) || (config->baudrate == 0U)) {
        return PLATFORM_INVALID_ARG;
    }

    if (uart != &console_uart) {
        return PLATFORM_INVALID_ARG;
    }

    memset(&uart->handle, 0, sizeof(uart->handle));
    memset(&uart->tx_dma, 0, sizeof(uart->tx_dma));
    memset(&uart->rx_dma, 0, sizeof(uart->rx_dma));

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_HSI);

    uart->handle.Instance = USART1;
    uart->handle.Init.BaudRate = config->baudrate;
    uart->handle.Init.WordLength = map_word_length(config->data_bits);
    uart->handle.Init.StopBits = map_stop_bits(config->stop_bits);
    uart->handle.Init.Parity = map_parity(config->parity);
    uart->handle.Init.Mode = UART_MODE_TX_RX;
    uart->handle.Init.HwFlowCtl = map_flow_control(config->flow_control);
    uart->handle.Init.OverSampling = UART_OVERSAMPLING_16;
    uart->handle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    uart->handle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    uart->handle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    platform_status_t status = map_hal_status(HAL_UART_Init(&uart->handle));
    if (status != PLATFORM_OK) {
        return status;
    }

    status = map_hal_status(HAL_UARTEx_SetTxFifoThreshold(&uart->handle, UART_TXFIFO_THRESHOLD_1_8));
    if (status != PLATFORM_OK) {
        return status;
    }

    status = map_hal_status(HAL_UARTEx_SetRxFifoThreshold(&uart->handle, UART_RXFIFO_THRESHOLD_1_8));
    if (status != PLATFORM_OK) {
        return status;
    }

    status = map_hal_status(HAL_UARTEx_DisableFifoMode(&uart->handle));
    if (status != PLATFORM_OK) {
        return status;
    }

    status = init_console_dma(uart);
    if (status != PLATFORM_OK) {
        return status;
    }

    uart->config = *config;
    uart->initialized = 1U;

    return PLATFORM_OK;
}

platform_status_t platform_uart_deinit(platform_uart_t *uart)
{
    if (uart == NULL) {
        return PLATFORM_INVALID_ARG;
    }

    uart->initialized = 0U;
    return map_hal_status(HAL_UART_DeInit(&uart->handle));
}

platform_status_t platform_uart_transmit(platform_uart_t *uart, const uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if ((uart == NULL) || ((data == NULL) && (length > 0U)) || (length > UINT16_MAX)) {
        return PLATFORM_INVALID_ARG;
    }

    if (length == 0U) {
        return PLATFORM_OK;
    }

    return map_hal_status(HAL_UART_Transmit(&uart->handle, (uint8_t *)data, (uint16_t)length, timeout_ms));
}

platform_status_t platform_uart_receive(platform_uart_t *uart, uint8_t *data, size_t length, uint32_t timeout_ms)
{
    if ((uart == NULL) || ((data == NULL) && (length > 0U)) || (length > UINT16_MAX)) {
        return PLATFORM_INVALID_ARG;
    }

    if (length == 0U) {
        return PLATFORM_OK;
    }

    return map_hal_status(HAL_UART_Receive(&uart->handle, data, (uint16_t)length, timeout_ms));
}

platform_status_t platform_uart_transmit_async(platform_uart_t *uart, const uint8_t *data, size_t length)
{
    if ((uart == NULL) || ((data == NULL) && (length > 0U)) || (length > UINT16_MAX)) {
        return PLATFORM_INVALID_ARG;
    }

    if (length == 0U) {
        return PLATFORM_OK;
    }

    return map_hal_status(HAL_UART_Transmit_DMA(&uart->handle, data, (uint16_t)length));
}

platform_status_t platform_uart_receive_async(platform_uart_t *uart, uint8_t *data, size_t length)
{
    if ((uart == NULL) || ((data == NULL) && (length > 0U)) || (length > UINT16_MAX)) {
        return PLATFORM_INVALID_ARG;
    }

    if (length == 0U) {
        return PLATFORM_OK;
    }

    uart->rx_dma_length = length;
    return map_hal_status(HAL_UART_Receive_DMA(&uart->handle, data, (uint16_t)length));
}

platform_status_t platform_uart_abort(platform_uart_t *uart)
{
    if (uart == NULL) {
        return PLATFORM_INVALID_ARG;
    }

    return map_hal_status(HAL_UART_Abort(&uart->handle));
}

platform_status_t platform_uart_abort_transmit(platform_uart_t *uart)
{
    if (uart == NULL) {
        return PLATFORM_INVALID_ARG;
    }

    return map_hal_status(HAL_UART_AbortTransmit(&uart->handle));
}

platform_status_t platform_uart_abort_receive(platform_uart_t *uart)
{
    if (uart == NULL) {
        return PLATFORM_INVALID_ARG;
    }

    return map_hal_status(HAL_UART_AbortReceive(&uart->handle));
}

platform_status_t platform_uart_register_callbacks(platform_uart_t *uart, const platform_uart_callbacks_t *callbacks)
{
    if (uart == NULL) {
        return PLATFORM_INVALID_ARG;
    }

    if (callbacks == NULL) {
        memset(&uart->callbacks, 0, sizeof(uart->callbacks));
    } else {
        uart->callbacks = *callbacks;
    }

    return PLATFORM_OK;
}

void HAL_UART_MspInit(UART_HandleTypeDef *uart_handle)
{
    GPIO_InitTypeDef gpio = {0};

    if (uart_handle->Instance == USART1) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

        gpio.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF7_USART1;

        HAL_GPIO_Init(GPIOA, &gpio);

        HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&console_uart.handle);
}

void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&console_uart.tx_dma);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&console_uart.rx_dma);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart_handle)
{
    platform_uart_t *uart = context_from_handle(uart_handle);

    if ((uart != NULL) && (uart->callbacks.tx_complete != NULL)) {
        uart->callbacks.tx_complete(uart, uart->callbacks.user_context);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart_handle)
{
    platform_uart_t *uart = context_from_handle(uart_handle);

    if ((uart != NULL) && (uart->callbacks.rx_complete != NULL)) {
        uart->callbacks.rx_complete(uart, uart->callbacks.user_context, uart->rx_dma_length);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart_handle)
{
    platform_uart_t *uart = context_from_handle(uart_handle);

    if ((uart != NULL) && (uart->callbacks.error != NULL)) {
        uart->callbacks.error(uart, uart->callbacks.user_context, PLATFORM_ERROR);
    }
}
