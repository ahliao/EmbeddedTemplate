#ifndef PLATFORM_UART_TYPES_H
#define PLATFORM_UART_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file platform_uart_types.h
 * @brief Target-independent UART interface types.
 *
 * This header is safe for application code and host-side tests to include.
 * It intentionally contains no STM32 handles, registers, IRQ names, or HAL/LL
 * types so UART users can be compiled against mocks.
 */

/** @brief Use with blocking APIs when the caller wants to wait indefinitely. */
#define PLATFORM_WAIT_FOREVER UINT32_MAX

/** @brief Common status values returned by platform interface functions. */
typedef enum {
    /** Operation completed successfully. */
    PLATFORM_OK = 0,
    /** Operation failed for a peripheral-specific reason. */
    PLATFORM_ERROR,
    /** Peripheral or transfer engine is already busy. */
    PLATFORM_BUSY,
    /** Operation did not complete before the requested timeout. */
    PLATFORM_TIMEOUT,
    /** A required pointer, size, or configuration value was invalid. */
    PLATFORM_INVALID_ARG
} platform_status_t;

/** @brief Logical UARTs exposed by the board support package. */
typedef enum {
    /** Console/debug UART selected by the active target. */
    PLATFORM_UART_ID_CONSOLE = 0,
    /** Number of logical UART identifiers. */
    PLATFORM_UART_ID_COUNT
} platform_uart_id_t;

/** @brief UART frame data width. */
typedef enum {
    /** 8 data bits per frame. */
    PLATFORM_UART_DATA_BITS_8 = 8,
    /** 9 data bits per frame. Not all backends support 9-bit DMA buffers. */
    PLATFORM_UART_DATA_BITS_9 = 9
} platform_uart_data_bits_t;

/** @brief UART parity selection. */
typedef enum {
    /** No parity bit. */
    PLATFORM_UART_PARITY_NONE = 0,
    /** Even parity bit. */
    PLATFORM_UART_PARITY_EVEN,
    /** Odd parity bit. */
    PLATFORM_UART_PARITY_ODD
} platform_uart_parity_t;

/** @brief UART stop bit selection. */
typedef enum {
    /** One stop bit. */
    PLATFORM_UART_STOP_BITS_1 = 0,
    /** Two stop bits. */
    PLATFORM_UART_STOP_BITS_2
} platform_uart_stop_bits_t;

/** @brief UART hardware flow-control selection. */
typedef enum {
    /** RTS/CTS disabled. */
    PLATFORM_UART_FLOW_CONTROL_NONE = 0,
    /** RTS/CTS enabled when supported by the backend and board routing. */
    PLATFORM_UART_FLOW_CONTROL_RTS_CTS
} platform_uart_flow_control_t;

/**
 * @brief Target-independent UART configuration.
 *
 * Hardware backends own clock source, pin mux, IRQ, and transfer engine setup.
 */
typedef struct {
    /** Requested baud rate in bits per second. */
    uint32_t baudrate;
    /** Number of data bits in each frame. */
    platform_uart_data_bits_t data_bits;
    /** Parity mode. */
    platform_uart_parity_t parity;
    /** Stop bit mode. */
    platform_uart_stop_bits_t stop_bits;
    /** Hardware flow-control mode. */
    platform_uart_flow_control_t flow_control;
} platform_uart_config_t;

/**
 * @brief Opaque UART handle used by application modules.
 *
 * Concrete backends define storage for this handle. Application code should
 * pass pointers through this interface and never inspect backend fields.
 */
typedef struct platform_uart platform_uart_t;

/** @brief Called when an asynchronous transmit operation completes. */
typedef void (*platform_uart_tx_complete_cb_t)(platform_uart_t *uart, void *user_context);

/** @brief Called when an asynchronous receive operation completes. */
typedef void (*platform_uart_rx_complete_cb_t)(platform_uart_t *uart, void *user_context, size_t bytes_received);

/** @brief Called when the backend detects a UART or DMA error. */
typedef void (*platform_uart_error_cb_t)(platform_uart_t *uart, void *user_context, platform_status_t error);

/**
 * @brief Optional callbacks for asynchronous UART operations.
 *
 * Register all callbacks with @ref platform_uart_register_callbacks. Any callback
 * may be NULL. The backend stores `user_context` and passes it back unchanged.
 */
typedef struct {
    /** Optional TX complete callback. */
    platform_uart_tx_complete_cb_t tx_complete;
    /** Optional RX complete callback. */
    platform_uart_rx_complete_cb_t rx_complete;
    /** Optional error callback. */
    platform_uart_error_cb_t error;
    /** Caller-owned pointer passed to every callback. */
    void *user_context;
} platform_uart_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_UART_TYPES_H */
