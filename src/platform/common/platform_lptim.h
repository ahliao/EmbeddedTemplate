#ifndef PLATFORM_LPTIM_INTERFACE_H
#define PLATFORM_LPTIM_INTERFACE_H

#include "platform_lptim_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file platform_lptim.h
 * @brief Mockable, target-independent low-power timer operations.
 */

/**
 * @brief Return a board-provided low-power timer handle for a logical identifier.
 *
 * @param id Logical timer selected by application code.
 * @return Opaque timer handle, or NULL when the identifier is unavailable.
 */
platform_lptim_t *platform_lptim_get(platform_lptim_id_t id);

/**
 * @brief Configure a low-power timer instance.
 *
 * @param timer Opaque backend timer handle.
 * @param config Target-independent timer configuration.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_lptim_init(platform_lptim_t *timer, const platform_lptim_config_t *config);

/**
 * @brief Disable and release backend timer state where supported.
 *
 * @param timer Opaque backend timer handle.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_lptim_deinit(platform_lptim_t *timer);

/**
 * @brief Start periodic timer interrupts.
 *
 * @param timer Opaque backend timer handle.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_lptim_start(platform_lptim_t *timer);

/**
 * @brief Stop periodic timer interrupts.
 *
 * @param timer Opaque backend timer handle.
 * @return PLATFORM_OK on success, otherwise an error status.
 */
platform_status_t platform_lptim_stop(platform_lptim_t *timer);

/**
 * @brief Register callbacks for low-power timer events.
 *
 * Passing NULL clears all callbacks. Callback execution context is backend and
 * board dependent; on embedded targets callbacks may run from ISR-adjacent code.
 * Keep callbacks short and defer heavy work to tasks.
 *
 * @param timer Opaque backend timer handle.
 * @param callbacks Callback table to copy, or NULL to clear callbacks.
 * @return PLATFORM_OK on success, otherwise PLATFORM_INVALID_ARG.
 */
platform_status_t platform_lptim_register_callbacks(platform_lptim_t *timer,
                                                    const platform_lptim_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_LPTIM_INTERFACE_H */
