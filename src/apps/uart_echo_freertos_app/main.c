#include "FreeRTOS.h"
#include "platform_uart.h"
#include "stm32u5xx_hal.h"
#include "task.h"

#define RX_BUFFER_SIZE 128U
#define RX_TIMEOUT_BITS 30U

static platform_uart_t *console_uart;
static uint8_t rxBuffer[RX_BUFFER_SIZE];
static size_t rxLength;
static volatile uint8_t uartError = 0;
static TaskHandle_t echoTaskHandle;

void Error_Handler(void);
void hal_freertos_timebase_scheduler_started(void);

static void EchoTask(void *argument);
static void StartReceiveAsync(void);
static void OnConsoleTxComplete(platform_uart_t *uart, void *user_context);
static void OnConsoleRxComplete(platform_uart_t *uart, void *user_context, size_t bytes_received);
static void OnConsoleError(platform_uart_t *uart, void *user_context, platform_status_t error);
static void InitConsoleUart(void);

int main(void)
{
    HAL_Init();

    if (xTaskCreate(EchoTask, "echo", configMINIMAL_STACK_SIZE * 2U, NULL, tskIDLE_PRIORITY + 1U, &echoTaskHandle) != pdPASS) {
        Error_Handler();
    }

    hal_freertos_timebase_scheduler_started();
    vTaskStartScheduler();

    Error_Handler();
}

static void EchoTask(void *argument)
{
    (void)argument;

    InitConsoleUart();

    const uint8_t banner[] = "\r\nFreeRTOS RTO console echo ready. Input is echoed after a short RX idle gap.\r\n";
    if (platform_uart_transmit(console_uart, banner, sizeof(banner) - 1, PLATFORM_WAIT_FOREVER) != PLATFORM_OK) {
        Error_Handler();
    }

    StartReceiveAsync();
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if ((rxLength > 0U) && (platform_uart_transmit(console_uart, rxBuffer, rxLength, PLATFORM_WAIT_FOREVER) != PLATFORM_OK)) {
            Error_Handler();
        }

        StartReceiveAsync();
    }
}

static void InitConsoleUart(void)
{
    console_uart = platform_uart_get(PLATFORM_UART_ID_CONSOLE);
    if (console_uart == NULL) {
        Error_Handler();
    }

    const platform_uart_callbacks_t callbacks = {
        .tx_complete = OnConsoleTxComplete,
        .rx_complete = OnConsoleRxComplete,
        .error = OnConsoleError,
        .user_context = NULL,
    };

    if (platform_uart_register_callbacks(console_uart, &callbacks) != PLATFORM_OK) {
        Error_Handler();
    }

    const platform_uart_config_t config = {
        .baudrate = 115200,
        .data_bits = PLATFORM_UART_DATA_BITS_8,
        .parity = PLATFORM_UART_PARITY_NONE,
        .stop_bits = PLATFORM_UART_STOP_BITS_1,
        .flow_control = PLATFORM_UART_FLOW_CONTROL_NONE,
    };

    if (platform_uart_init(console_uart, &config) != PLATFORM_OK) {
        Error_Handler();
    }
}

static void StartReceiveAsync(void)
{
    rxLength = 0U;
    if (platform_uart_receive_until_timeout_async(console_uart, rxBuffer, sizeof(rxBuffer), RX_TIMEOUT_BITS) != PLATFORM_OK) {
        Error_Handler();
    }
}

static void OnConsoleTxComplete(platform_uart_t *uart, void *user_context)
{
    (void)uart;
    (void)user_context;
}

static void OnConsoleRxComplete(platform_uart_t *uart, void *user_context, size_t bytes_received)
{
    (void)uart;
    (void)user_context;

    BaseType_t higher_priority_task_woken = pdFALSE;

    if (bytes_received > sizeof(rxBuffer)) {
        Error_Handler();
    }

    rxLength = bytes_received;
    vTaskNotifyGiveFromISR(echoTaskHandle, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

static void OnConsoleError(platform_uart_t *uart, void *user_context, platform_status_t error)
{
    (void)uart;
    (void)user_context;
    (void)error;

    uartError++;
    Error_Handler();
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}

void vApplicationMallocFailedHook(void)
{
    Error_Handler();
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    (void)task_name;

    Error_Handler();
}
