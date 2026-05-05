#include "FreeRTOS.h"
#include "platform_uart.h"
#include "stm32u5xx_hal.h"
#include "task.h"

static platform_uart_t *console_uart;
static uint8_t rxByte;
static volatile uint8_t rxDone = 0;
static volatile uint8_t txDone = 0;
static volatile uint8_t uartError = 0;
static TaskHandle_t echoTaskHandle;

void Error_Handler(void);

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

    vTaskStartScheduler();

    Error_Handler();
}

static void EchoTask(void *argument)
{
    (void)argument;

    InitConsoleUart();

    const uint8_t banner[] = "\r\nFreeRTOS async console echo ready. Type characters and they will echo back.\r\n";
    if (platform_uart_transmit(console_uart, banner, sizeof(banner) - 1, PLATFORM_WAIT_FOREVER) != PLATFORM_OK) {
        Error_Handler();
    }

    StartReceiveAsync();

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
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
    if (platform_uart_receive_async(console_uart, &rxByte, 1) != PLATFORM_OK) {
        Error_Handler();
    }
}

static void OnConsoleTxComplete(platform_uart_t *uart, void *user_context)
{
    (void)uart;
    (void)user_context;

    txDone++;
    StartReceiveAsync();
}

static void OnConsoleRxComplete(platform_uart_t *uart, void *user_context, size_t bytes_received)
{
    (void)user_context;

    BaseType_t higher_priority_task_woken = pdFALSE;

    rxDone++;

    if ((bytes_received != 1U) || (platform_uart_transmit_async(uart, &rxByte, 1) != PLATFORM_OK)) {
        Error_Handler();
    }

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
