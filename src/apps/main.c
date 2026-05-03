#include "stm32u5xx_hal.h"
#include "stm32u5xx_ll_rcc.h"

UART_HandleTypeDef huart1;

DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;

static uint8_t rxByte;
static volatile uint8_t rxDone = 0;
static volatile uint8_t txDone = 0;
static volatile uint8_t uartError = 0;

void Error_Handler(void);

static void MX_USART1_UART_Init(void);
static void MX_GPDMA1_Init(void);
static void StartReceiveDma(void);

int main(void)
{
    HAL_Init();

    /*
     * Match your Renode USART frequency to HSI, e.g.:
     * usart1 frequency: 16000000
     */
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_HSI);

    MX_USART1_UART_Init();
    MX_GPDMA1_Init();

    const uint8_t banner[] =
        "\r\nDMA USART1 echo ready. Type characters and they will echo back.\r\n";

    /*
     * Optional startup message using blocking TX.
     * This confirms basic TX works before testing DMA RX/TX echo.
     */
    HAL_UART_Transmit(&huart1, (uint8_t *)banner, sizeof(banner) - 1, HAL_MAX_DELAY);

    StartReceiveDma();

    while(1)
    {
        /*
         * Main loop intentionally does almost nothing.
         * Echo is handled by DMA completion callbacks.
         *
         * Useful for debugger:
         *   rxDone increments when RX DMA finishes.
         *   txDone increments when TX DMA finishes.
         *   uartError increments if HAL reports an error.
         */
    }
}

static void StartReceiveDma(void)
{
    if(HAL_UART_Receive_DMA(&huart1, &rxByte, 1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if(HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Keep FIFO disabled for basic Renode USART/DMA bring-up.
     */
    if(HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if(HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
    {
        Error_Handler();
    }

    if(HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPDMA1_Init(void)
{
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    /*
     * Channel 0: memory -> USART1 TDR
     */
    hdma_usart1_tx.Instance = GPDMA1_Channel0;
    hdma_usart1_tx.Init.Request = GPDMA1_REQUEST_USART1_TX;
    hdma_usart1_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    hdma_usart1_tx.Init.DestInc = DMA_DINC_FIXED;
    hdma_usart1_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_usart1_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_usart1_tx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    hdma_usart1_tx.Init.SrcBurstLength = 1;
    hdma_usart1_tx.Init.DestBurstLength = 1;
    hdma_usart1_tx.Init.TransferAllocatedPort =
        DMA_SRC_ALLOCATED_PORT1 | DMA_DEST_ALLOCATED_PORT0;
    hdma_usart1_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;

    if(HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Channel 1: USART1 RDR -> memory
     */
    hdma_usart1_rx.Instance = GPDMA1_Channel1;
    hdma_usart1_rx.Init.Request = GPDMA1_REQUEST_USART1_RX;
    hdma_usart1_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.SrcInc = DMA_SINC_FIXED;
    hdma_usart1_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    hdma_usart1_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma_usart1_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma_usart1_rx.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    hdma_usart1_rx.Init.SrcBurstLength = 1;
    hdma_usart1_rx.Init.DestBurstLength = 1;
    hdma_usart1_rx.Init.TransferAllocatedPort =
        DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
    hdma_usart1_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;

    if(HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Associate DMA handles with UART handle.
     */
    __HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);
    __HAL_LINKDMA(&huart1, hdmarx, hdma_usart1_rx);

    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(uartHandle->Instance == USART1)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;

        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /*
         * Keep USART IRQ enabled. STM32 HAL UART DMA TX completion may use
         * the USART IRQ path after DMA transfer complete.
         */
        HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_tx);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        rxDone++;

        /*
         * Echo received byte using TX DMA.
         * Do not restart RX here yet; wait until TX finishes for simplest debugging.
         */
        if(HAL_UART_Transmit_DMA(&huart1, &rxByte, 1) != HAL_OK)
        {
            Error_Handler();
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        txDone++;

        /*
         * After echoing one byte, arm RX DMA again.
         */
        StartReceiveDma();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        uartError++;
    }

    Error_Handler();
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void Error_Handler(void)
{
    __disable_irq();
    while(1)
    {
    }
}