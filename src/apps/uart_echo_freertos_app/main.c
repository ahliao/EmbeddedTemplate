#include "platform_lptim.h"
#include "platform_uart.h"
// #include "stm32u5xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>

#define RX_BUFFER_SIZE 128U
#define RX_TIMEOUT_BITS 30U
#define HELLO_PERIOD_MS 5000U
#define ECHO_NOTIFY_RX (1UL << 0)
#define ECHO_NOTIFY_PERIODIC_TX (1UL << 1)

static platform_uart_t *console_uart;
static platform_lptim_t *periodic_lptim;
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
static void OnPeriodicTimerElapsed(platform_lptim_t *timer, void *user_context);
static void InitConsoleUart(void);
static void InitPeriodicLptim(void);

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

    InitPeriodicLptim();

    StartReceiveAsync();
    uint32_t notifications = 0;
    while (1) {
        (void)xTaskNotifyWait(0U, UINT32_MAX, &notifications, portMAX_DELAY);

        if ((notifications & ECHO_NOTIFY_RX) != 0U) {
            if ((rxLength > 0U) && (platform_uart_transmit(console_uart, rxBuffer, rxLength, PLATFORM_WAIT_FOREVER) != PLATFORM_OK)) {
                Error_Handler();
            }

            StartReceiveAsync();
        }

        if ((notifications & ECHO_NOTIFY_PERIODIC_TX) != 0U) {
            const uint8_t hello[] = "\r\nHello World\r\n";
            if (platform_uart_transmit(console_uart, hello, sizeof(hello) - 1U, PLATFORM_WAIT_FOREVER) != PLATFORM_OK) {
                Error_Handler();
            }
        }
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

static void InitPeriodicLptim(void)
{
    periodic_lptim = platform_lptim_get(PLATFORM_LPTIM_ID_PERIODIC);
    if (periodic_lptim == NULL) {
        Error_Handler();
    }

    const platform_lptim_callbacks_t callbacks = {
        .period_elapsed = OnPeriodicTimerElapsed,
        .user_context = NULL,
    };

    if (platform_lptim_register_callbacks(periodic_lptim, &callbacks) != PLATFORM_OK) {
        Error_Handler();
    }

    const platform_lptim_config_t config = {
        .period_ms = HELLO_PERIOD_MS,
    };

    if (platform_lptim_init(periodic_lptim, &config) != PLATFORM_OK) {
        Error_Handler();
    }

    if (platform_lptim_start(periodic_lptim) != PLATFORM_OK) {
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
    (void)xTaskNotifyFromISR(echoTaskHandle, ECHO_NOTIFY_RX, eSetBits, &higher_priority_task_woken);
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

static void OnPeriodicTimerElapsed(platform_lptim_t *timer, void *user_context)
{
    (void)timer;
    (void)user_context;

    BaseType_t higher_priority_task_woken = pdFALSE;

    (void)xTaskNotifyFromISR(echoTaskHandle, ECHO_NOTIFY_PERIODIC_TX, eSetBits, &higher_priority_task_woken);
    portYIELD_FROM_ISR(higher_priority_task_woken);
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
