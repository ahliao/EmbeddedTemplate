add_library(freertos_config INTERFACE)
target_compile_definitions(freertos_config INTERFACE STM32U5A5xx)
target_include_directories(freertos_config INTERFACE "${APP_DIR}" ${MCU_INCLUDE_DIRS})

set(FREERTOS_PORT "GCC_ARM_CM33_NTZ_NONSECURE" CACHE STRING "FreeRTOS port name" FORCE)
set(FREERTOS_HEAP "4" CACHE STRING "FreeRTOS heap implementation" FORCE)

add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/FreeRTOS-Kernel" "${CMAKE_BINARY_DIR}/FreeRTOS-Kernel")

set(APP_SRCS
	"${APP_DIR}/main.c"
	"${APP_DIR}/hal_freertos_time.c"
)

set(APP_INCLUDE_DIRS
	"${APP_DIR}"
)

set(APP_LIBS
	freertos_kernel
	freertos_config
)
