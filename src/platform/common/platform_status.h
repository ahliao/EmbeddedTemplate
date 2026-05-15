#ifndef PLATFORM_STATUS_H
#define PLATFORM_STATUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_STATUS_H */
