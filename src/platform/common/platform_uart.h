#ifndef PLATFORM_UART_INTERFACE_H
#define PLATFORM_UART_INTERFACE_H

#include "platform_uart_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file platform_uart.h
 * @brief Mockable, target-independent UART operations.
 *
 * Application modules should depend on this header rather than STM32 HAL/LL
 * headers. Tests can provide alternate definitions for these functions or fake
 * `platform_uart_t` handles without including target-specific code.
 */

/**
 * @brief Return a board-provided UART handle for a logical UART identifier.
 *
 * @param id Logical UART selected by application code.
 * @return Opaque UART handle, or NULL when the identifier is unavailable.
 */
platform_uart_t *platform_uart_get(platform_uart_id_t id);

/**
 * @brief Configure and enable a UART instance.
 *
 * The caller must provide a backend-created `uart` handle and a valid config.
 * The backend owns board-specific pin mux, clocks, NVIC, and transfer engine
 * setup.
 *
 * @param uart Opaque backend UART handle.
 * @param config Target-independent UART configuration.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_uart_init(platform_uart_t *uart, const platform_uart_config_t *config);

/**
 * @brief Disable and release backend UART state where supported.
 *
 * @param uart Opaque backend UART handle.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_uart_deinit(platform_uart_t *uart);

/**
 * @brief Transmit bytes using a blocking polling path.
 *
 * The function returns after all bytes are written and final transmission is
 * complete, or earlier on timeout/error. A zero-length transfer succeeds.
 *
 * @param uart Opaque backend UART handle.
 * @param data Bytes to transmit. May be NULL only when length is zero.
 * @param length Number of bytes to transmit.
 * @param timeout_ms Timeout in milliseconds, or PLATFORM_WAIT_FOREVER.
 * @return PLATFORM_OK, PLATFORM_TIMEOUT, PLATFORM_INVALID_ARG, or PLATFORM_ERROR.
 */
platform_status_t platform_uart_transmit(platform_uart_t *uart, const uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Receive bytes using a blocking polling path.
 *
 * The function returns after `length` bytes have been received, or earlier on
 * timeout/error. A zero-length receive succeeds.
 *
 * @param uart Opaque backend UART handle.
 * @param data Destination buffer. May be NULL only when length is zero.
 * @param length Number of bytes to receive.
 * @param timeout_ms Timeout in milliseconds, or PLATFORM_WAIT_FOREVER.
 * @return PLATFORM_OK, PLATFORM_TIMEOUT, PLATFORM_INVALID_ARG, or PLATFORM_ERROR.
 */
platform_status_t platform_uart_receive(platform_uart_t *uart, uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Start an asynchronous transmit.
 *
 * The transmit buffer must remain valid until the TX complete callback fires or
 * the transfer is aborted. A zero-length transfer succeeds without starting a
 * transfer.
 *
 * @param uart Opaque backend UART handle.
 * @param data Bytes to transmit. May be NULL only when length is zero.
 * @param length Number of bytes to transmit.
 * @return PLATFORM_OK if the transfer was started, otherwise a status such as PLATFORM_BUSY.
 */
platform_status_t platform_uart_transmit_async(platform_uart_t *uart, const uint8_t *data, size_t length);

/**
 * @brief Start an asynchronous receive.
 *
 * The receive buffer must remain valid until the RX complete callback fires or
 * the transfer is aborted. A zero-length transfer succeeds without starting a
 * transfer.
 *
 * @param uart Opaque backend UART handle.
 * @param data Destination buffer. May be NULL only when length is zero.
 * @param length Number of bytes to receive.
 * @return PLATFORM_OK if the transfer was started, otherwise a status such as PLATFORM_BUSY.
 */
platform_status_t platform_uart_receive_async(platform_uart_t *uart, uint8_t *data, size_t length);

/**
 * @brief Abort any active UART transfer supported by the backend.
 *
 * @param uart Opaque backend UART handle.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_uart_abort(platform_uart_t *uart);

/**
 * @brief Abort an active transmit operation supported by the backend.
 *
 * @param uart Opaque backend UART handle.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_uart_abort_transmit(platform_uart_t *uart);

/**
 * @brief Abort an active receive operation supported by the backend.
 *
 * @param uart Opaque backend UART handle.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_uart_abort_receive(platform_uart_t *uart);

/**
 * @brief Register callbacks for asynchronous UART events.
 *
 * Passing NULL clears all callbacks. Callback execution context is backend and
 * board dependent; on embedded targets callbacks may run from ISR-adjacent code.
 * Keep callbacks short and defer heavy work to tasks.
 *
 * @param uart Opaque backend UART handle.
 * @param callbacks Callback table to copy, or NULL to clear callbacks.
 * @return PLATFORM_OK on success, otherwise PLATFORM_INVALID_ARG.
 */
platform_status_t platform_uart_register_callbacks(platform_uart_t *uart, const platform_uart_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_UART_INTERFACE_H */
