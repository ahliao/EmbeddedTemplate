#ifndef HAL_UART_STM32U5_H
#define HAL_UART_STM32U5_H

#include "../common/hal_uart_interface.h"

#include "stm32u5xx_hal.h"
#include "stm32u5xx_ll_usart.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file hal_uart_stm32u5.h
 * @brief STM32U5 backend storage and binding helpers for the common UART API.
 *
 * This target-specific header may include STM32 HAL/LL types. Application code
 * that only needs portable UART operations should include
 * `hal_uart_interface.h` instead. Board/platform code owns instances of
 * @ref hal_uart_stm32u5_t and passes them to application code as `hal_uart_t`.
 */

/**
 * @brief STM32U5 UART backend context.
 *
 * `instance` is required for LL-based blocking operations. `hal_handle` is
 * required for DMA and abort APIs because this backend delegates those paths to
 * STM32 HAL UART. The HAL handle must already be configured with any DMA handles
 * needed by the board.
 */
typedef struct {
    /** STM32 USART/LPUART register block used by LL operations. */
    USART_TypeDef *instance;
    /** Optional STM32 HAL UART handle used for DMA and abort operations. */
    UART_HandleTypeDef *hal_handle;
    /** Last configuration passed to hal_uart_init. */
    hal_uart_config_t config;
    /** Registered common-layer callbacks. */
    hal_uart_callbacks_t callbacks;
    /** Length requested by the most recent DMA receive. */
    size_t dma_rx_length;
} hal_uart_stm32u5_t;

/**
 * @brief Bind STM32U5 hardware objects to a UART backend context.
 *
 * Call this from board/platform initialization before passing the UART to
 * @ref hal_uart_init. `hal_handle` may be NULL if only LL blocking TX/RX and
 * deinit are used; DMA and abort APIs require it.
 *
 * @param context Caller-owned STM32U5 backend storage.
 * @param instance USART/LPUART peripheral instance.
 * @param hal_handle Optional STM32 HAL UART handle for DMA/abort paths.
 * @return HAL_IF_OK on success, otherwise HAL_IF_INVALID_ARG.
 */
hal_if_status_t hal_uart_stm32u5_bind(hal_uart_stm32u5_t *context,
                                      USART_TypeDef *instance,
                                      UART_HandleTypeDef *hal_handle);

/**
 * @brief Convert STM32U5 backend storage to the common opaque UART handle.
 *
 * @param context Caller-owned STM32U5 backend storage.
 * @return Opaque handle for use with `hal_uart_*` functions.
 */
hal_uart_t *hal_uart_stm32u5_as_handle(hal_uart_stm32u5_t *context);

/**
 * @brief Notify the common callback layer that DMA/interrupt TX completed.
 *
 * Board ISR glue or HAL callback code should call this after identifying the
 * owning @ref hal_uart_stm32u5_t instance.
 */
void hal_uart_stm32u5_on_tx_complete(hal_uart_stm32u5_t *context);

/**
 * @brief Notify the common callback layer that DMA/interrupt RX completed.
 *
 * @param context STM32U5 backend context.
 * @param bytes_received Number of bytes received by the completed operation.
 */
void hal_uart_stm32u5_on_rx_complete(hal_uart_stm32u5_t *context, size_t bytes_received);

/**
 * @brief Notify the common callback layer that a UART/DMA error occurred.
 *
 * @param context STM32U5 backend context.
 * @param error Common status value representing the failure.
 */
void hal_uart_stm32u5_on_error(hal_uart_stm32u5_t *context, hal_if_status_t error);

#ifdef __cplusplus
}
#endif

#endif /* HAL_UART_STM32U5_H */
