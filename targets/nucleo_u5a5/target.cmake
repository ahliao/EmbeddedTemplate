set(TARGET_NAME "nucleo_u5a5")
set(MCU_FAMILY "stm32u5")

set(MCU_CPU "cortex-m33")
set(MCU_FPU "auto")
set(MCU_FLOAT_ABI "hard")

set(MCU_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/targets/nucleo_u5a5/STM32U5A5xx_FLASH.ld")
set(MCU_STARTUP_FILE "${CMAKE_SOURCE_DIR}/submodules/CMSIS/cmsis-device-u5/Source/Templates/gcc/startup_stm32u5a5xx.s")
set(MCU_SYSTEM_FILE "${CMAKE_SOURCE_DIR}/submodules/CMSIS/cmsis-device-u5/Source/Templates/system_stm32u5xx.c")

set(MCU_DEFINES
    STM32U5A5xx
    USE_HAL_DRIVER
)

set(MCU_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/targets/nucleo_u5a5"
)
