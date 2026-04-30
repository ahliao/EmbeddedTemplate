#ifndef HAL_UART_TYPES_H
#define HAL_UART_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file hal_uart_types.h
 * @brief Target-independent UART interface types.
 *
 * This header is safe for application code and host-side tests to include.
 * It intentionally contains no STM32 handles, registers, IRQ names, or HAL/LL
 * types so UART users can be compiled against mocks.
 */

/** @brief Use with blocking APIs when the caller wants to wait indefinitely. */
#define HAL_IF_WAIT_FOREVER UINT32_MAX

/** @brief Common status values returned by HAL interface functions. */
typedef enum {
    /** Operation completed successfully. */
    HAL_IF_OK = 0,
    /** Operation failed for a peripheral-specific reason. */
    HAL_IF_ERROR,
    /** Peripheral or transfer engine is already busy. */
    HAL_IF_BUSY,
    /** Operation did not complete before the requested timeout. */
    HAL_IF_TIMEOUT,
    /** A required pointer, size, or configuration value was invalid. */
    HAL_IF_INVALID_ARG
} hal_if_status_t;

/** @brief UART frame data width. */
typedef enum {
    /** 8 data bits per frame. */
    HAL_IF_UART_DATA_BITS_8 = 8,
    /** 9 data bits per frame. Not all backends support 9-bit DMA buffers. */
    HAL_IF_UART_DATA_BITS_9 = 9
} hal_uart_data_bits_t;

/** @brief UART parity selection. */
typedef enum {
    /** No parity bit. */
    HAL_IF_UART_PARITY_NONE = 0,
    /** Even parity bit. */
    HAL_IF_UART_PARITY_EVEN,
    /** Odd parity bit. */
    HAL_IF_UART_PARITY_ODD
} hal_uart_parity_t;

/** @brief UART stop bit selection. */
typedef enum {
    /** One stop bit. */
    HAL_IF_UART_STOP_BITS_1 = 0,
    /** Two stop bits. */
    HAL_IF_UART_STOP_BITS_2
} hal_uart_stop_bits_t;

/** @brief UART hardware flow-control selection. */
typedef enum {
    /** RTS/CTS disabled. */
    HAL_IF_UART_FLOW_CONTROL_NONE = 0,
    /** RTS/CTS enabled when supported by the backend and board routing. */
    HAL_IF_UART_FLOW_CONTROL_RTS_CTS
} hal_uart_flow_control_t;

/**
 * @brief Target-independent UART configuration.
 *
 * `source_clock_hz` is the UART peripheral input clock used by hardware
 * backends to calculate baud-rate divisors. Board code owns clock enablement,
 * GPIO alternate-function setup, and DMA channel setup before calling init.
 */
typedef struct {
    /** Requested baud rate in bits per second. */
    uint32_t baudrate;
    /** UART peripheral source clock in Hz. */
    uint32_t source_clock_hz;
    /** Number of data bits in each frame. */
    hal_uart_data_bits_t data_bits;
    /** Parity mode. */
    hal_uart_parity_t parity;
    /** Stop bit mode. */
    hal_uart_stop_bits_t stop_bits;
    /** Hardware flow-control mode. */
    hal_uart_flow_control_t flow_control;
} hal_uart_config_t;

/**
 * @brief Opaque UART handle used by application modules.
 *
 * Concrete backends define storage for this handle. Application code should
 * pass pointers through this interface and never inspect backend fields.
 */
typedef struct hal_uart hal_uart_t;

/** @brief Called when an asynchronous transmit operation completes. */
typedef void (*hal_uart_tx_complete_cb_t)(hal_uart_t *uart, void *user_context);

/** @brief Called when an asynchronous receive operation completes. */
typedef void (*hal_uart_rx_complete_cb_t)(hal_uart_t *uart, void *user_context, size_t bytes_received);

/** @brief Called when the backend detects a UART or DMA error. */
typedef void (*hal_uart_error_cb_t)(hal_uart_t *uart, void *user_context, hal_if_status_t error);

/**
 * @brief Optional callbacks for asynchronous UART operations.
 *
 * Register all callbacks with @ref hal_uart_register_callbacks. Any callback
 * may be NULL. The backend stores `user_context` and passes it back unchanged.
 */
typedef struct {
    /** Optional TX complete callback. */
    hal_uart_tx_complete_cb_t tx_complete;
    /** Optional RX complete callback. */
    hal_uart_rx_complete_cb_t rx_complete;
    /** Optional error callback. */
    hal_uart_error_cb_t error;
    /** Caller-owned pointer passed to every callback. */
    void *user_context;
} hal_uart_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_TYPES_H */
