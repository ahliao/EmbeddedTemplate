#ifndef PLATFORM_LPTIM_TYPES_H
#define PLATFORM_LPTIM_TYPES_H

#include "platform_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file platform_lptim_types.h
 * @brief Target-independent low-power timer interface types.
 */

/** @brief Logical low-power timers exposed by the board support package. */
typedef enum {
    /** Periodic application timer selected by the active target. */
    PLATFORM_LPTIM_ID_PERIODIC = 0,
    /** Number of logical low-power timer identifiers. */
    PLATFORM_LPTIM_ID_COUNT
} platform_lptim_id_t;

/**
 * @brief Target-independent low-power timer configuration.
 *
 * Hardware backends own clock source, prescaler selection, IRQ, and counter
 * setup. The requested period is expressed in milliseconds.
 */
typedef struct {
    /** Requested periodic interval in milliseconds. */
    uint32_t period_ms;
} platform_lptim_config_t;

/** @brief Opaque low-power timer handle used by application modules. */
typedef struct platform_lptim platform_lptim_t;

/** @brief Called when a periodic low-power timer interval elapses. */
typedef void (*platform_lptim_period_elapsed_cb_t)(platform_lptim_t *timer, void *user_context);

/** @brief Optional callbacks for low-power timer events. */
typedef struct {
    /** Optional period elapsed callback. */
    platform_lptim_period_elapsed_cb_t period_elapsed;
    /** Caller-owned pointer passed to every callback. */
    void *user_context;
} platform_lptim_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_LPTIM_TYPES_H */
