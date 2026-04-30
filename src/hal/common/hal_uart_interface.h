#ifndef HAL_UART_INTERFACE_H
#define HAL_UART_INTERFACE_H

#include "hal_uart_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file hal_uart_interface.h
 * @brief Mockable, target-independent UART operations.
 *
 * Application modules should depend on this header rather than STM32 HAL/LL
 * headers. Tests can provide alternate definitions for these functions or fake
 * `hal_uart_t` handles without including target-specific code.
 */

/**
 * @brief Configure and enable a UART instance.
 *
 * The caller must provide a backend-created `uart` handle and a valid config.
 * Board-specific pin mux, clocks, NVIC, and DMA channel setup are outside this
 * common interface and must be done by board/platform code.
 *
 * @param uart Opaque backend UART handle.
 * @param config Target-independent UART configuration.
 * @return HAL_IF_OK on success, otherwise an error status.
 */
hal_if_status_t hal_uart_init(hal_uart_t *uart, const hal_uart_config_t *config);

/**
 * @brief Disable and release backend UART state where supported.
 *
 * @param uart Opaque backend UART handle.
 * @return HAL_IF_OK on success, otherwise an error status.
 */
hal_if_status_t hal_uart_deinit(hal_uart_t *uart);

/**
 * @brief Transmit bytes using a blocking polling path.
 *
 * The function returns after all bytes are written and final transmission is
 * complete, or earlier on timeout/error. A zero-length transfer succeeds.
 *
 * @param uart Opaque backend UART handle.
 * @param data Bytes to transmit. May be NULL only when length is zero.
 * @param length Number of bytes to transmit.
 * @param timeout_ms Timeout in milliseconds, or HAL_IF_WAIT_FOREVER.
 * @return HAL_IF_OK, HAL_IF_TIMEOUT, HAL_IF_INVALID_ARG, or HAL_IF_ERROR.
 */
hal_if_status_t hal_uart_transmit(hal_uart_t *uart, const uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Receive bytes using a blocking polling path.
 *
 * The function returns after `length` bytes have been received, or earlier on
 * timeout/error. A zero-length receive succeeds.
 *
 * @param uart Opaque backend UART handle.
 * @param data Destination buffer. May be NULL only when length is zero.
 * @param length Number of bytes to receive.
 * @param timeout_ms Timeout in milliseconds, or HAL_IF_WAIT_FOREVER.
 * @return HAL_IF_OK, HAL_IF_TIMEOUT, HAL_IF_INVALID_ARG, or HAL_IF_ERROR.
 */
hal_if_status_t hal_uart_receive(hal_uart_t *uart, uint8_t *data, size_t length, uint32_t timeout_ms);

/**
 * @brief Start an asynchronous DMA transmit.
 *
 * The transmit buffer must remain valid until the TX complete callback fires or
 * the transfer is aborted. A zero-length transfer succeeds without starting DMA.
 *
 * @param uart Opaque backend UART handle.
 * @param data Bytes to transmit. May be NULL only when length is zero.
 * @param length Number of bytes to transmit.
 * @return HAL_IF_OK if DMA was started, otherwise a status such as HAL_IF_BUSY.
 */
hal_if_status_t hal_uart_transmit_dma(hal_uart_t *uart, const uint8_t *data, size_t length);

/**
 * @brief Start an asynchronous DMA receive.
 *
 * The receive buffer must remain valid until the RX complete callback fires or
 * the transfer is aborted. A zero-length transfer succeeds without starting DMA.
 *
 * @param uart Opaque backend UART handle.
 * @param data Destination buffer. May be NULL only when length is zero.
 * @param length Number of bytes to receive.
 * @return HAL_IF_OK if DMA was started, otherwise a status such as HAL_IF_BUSY.
 */
hal_if_status_t hal_uart_receive_dma(hal_uart_t *uart, uint8_t *data, size_t length);

/**
 * @brief Abort any active UART transfer supported by the backend.
 *
 * @param uart Opaque backend UART handle.
 * @return HAL_IF_OK on success, otherwise an error status.
 */
hal_if_status_t hal_uart_abort(hal_uart_t *uart);

/**
 * @brief Abort an active transmit operation supported by the backend.
 *
 * @param uart Opaque backend UART handle.
 * @return HAL_IF_OK on success, otherwise an error status.
 */
hal_if_status_t hal_uart_abort_transmit(hal_uart_t *uart);

/**
 * @brief Abort an active receive operation supported by the backend.
 *
 * @param uart Opaque backend UART handle.
 * @return HAL_IF_OK on success, otherwise an error status.
 */
hal_if_status_t hal_uart_abort_receive(hal_uart_t *uart);

/**
 * @brief Register callbacks for asynchronous UART events.
 *
 * Passing NULL clears all callbacks. Callback execution context is backend and
 * board dependent; on embedded targets callbacks may run from ISR-adjacent code.
 * Keep callbacks short and defer heavy work to tasks.
 *
 * @param uart Opaque backend UART handle.
 * @param callbacks Callback table to copy, or NULL to clear callbacks.
 * @return HAL_IF_OK on success, otherwise HAL_IF_INVALID_ARG.
 */
hal_if_status_t hal_uart_register_callbacks(hal_uart_t *uart, const hal_uart_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_INTERFACE_H */
