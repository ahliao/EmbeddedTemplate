# AGENTS.md

## Build And Verify
- Initialize vendor code before configuring: `git submodule update --init --recursive`.
- Preferred configure/build path is the preset: `cmake --workflow --preset nucleo_u5a5`.
- Equivalent split commands: `cmake --preset nucleo_u5a5` then `cmake --build --preset nucleo_u5a5`.
- The default toolchain file is `cmake/cross.cmake`; it expects GNU Arm tools under `/opt/arm-gnu/bin`. Override with `-DARM_NONE_EABI_ROOT=/path` or `-DCMAKE_TOOLCHAIN_FILE=/path/to/toolchain.cmake` if needed.
- No test, lint, format, flash, or debug target is defined in the repo right now; do not invent one when reporting verification.

## Project Shape
- Firmware target is `embedded_template.elf`; CMake writes outputs under the preset build dir at `build/nucleo_u5a5/build/`.
- `CMakeLists.txt` selects a board via `EMBEDDED_TARGET`, then includes `targets/${EMBEDDED_TARGET}/target.cmake` for CPU flags, defines, linker script, startup file, system file, and board includes.
- The only configured target today is `nucleo_u5a5`, an STM32U5A5/Cortex-M33 target using `targets/nucleo_u5a5/STM32U5A5xx_FLASH.ld` and HAL config in `targets/nucleo_u5a5/stm32u5xx_hal_conf.h`.
- Application sources are globbed recursively from `src/` for C, C++, and assembly; the current entry point is `src/apps/main.c`.
- STM32U5 HAL is built as the `hal_stm32_u5` object library from `submodules/STM32_HAL`; CMSIS include/startup/system files come from `submodules/CMSIS/cmsis-core` and `submodules/CMSIS/cmsis-device-u5`.

## Gotchas
- README layout notes mention `submodules/CMSIS_6`, but the active CMake build uses `submodules/CMSIS/...`; trust the CMake files over README prose if they differ.
- `MCU_FPU` is `auto` for `nucleo_u5a5`, so CMake intentionally skips `-mfpu` and `-mfloat-abi` flags even though `MCU_FLOAT_ABI` is set.
- If adding a new board, create `targets/<name>/target.cmake` and set `EMBEDDED_TARGET=<name>`; unsupported `MCU_FAMILY` values fail at configure time.
