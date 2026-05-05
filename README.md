# EmbeddedTemplate

Template for STM32 Cortex-M embedded firmware projects using CMake and the GNU Arm Embedded toolchain.

The repository provides a small application entry point, a CMake build configuration, and CMSIS 6 as a Git submodule. It is intended as a starting point that you customize for a specific MCU, board, linker script, startup file, and peripheral stack.

## Requirements

- CMake 3.13 or newer
- A C/C++ build backend supported by CMake, such as Ninja or Make
- `arm-none-eabi-gcc` on your `PATH`, or an explicit CMake toolchain/compiler configuration
- Git submodule support for fetching CMSIS 6

## Repository Layout

```text
.
|-- CMakeLists.txt          # Main firmware build configuration
|-- cmake/                  # CMake helper files
|-- src/apps/main.c         # Minimal application entry point
`-- submodules/CMSIS_6/     # ARM CMSIS 6 submodule
```

## Setup

Clone the repository with submodules:

```sh
git clone --recurse-submodules <repository-url>
cd EmbeddedTemplate
```

If you already cloned without submodules, initialize them with:

```sh
git submodule update --init --recursive
```

## Configure

Create a build directory and configure CMake:

```sh
cmake -S . -B build
```

The default target CPU is `cortex-m4`. You can override MCU settings from the command line:

```sh
cmake -S . -B build \
  -DMCU_CPU=cortex-m4 \
  -DMCU_FPU=fpv4-sp-d16 \
  -DMCU_FLOAT_ABI=hard
```

You can also provide an explicit toolchain file:

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=path/to/arm-none-eabi.cmake
```

## Build

Build the firmware target:

```sh
cmake --build build
```

The configured executable target is `embedded_template.elf`. Build artifacts are written under `build/build/` by the current CMake configuration.

When `arm-none-eabi-objcopy` is available, the build also generates:

- `embedded_template.bin`
- `embedded_template.hex`

When `arm-none-eabi-size` is available, the final ELF size is printed after linking.

## Debug With Renode And VSCode

The repository includes a VSCode launch configuration for debugging the non-RTOS `nucleo_u5a5` UART echo app in Renode.

Requirements:

- Custom Renode checkout available at `../renode`
- GNU Arm tools under `/opt/arm-gnu/bin`, including `arm-none-eabi-gdb`
- VSCode Cortex-Debug extension

Start debugging from VSCode with the `Renode: Debug Nucleo U5A5 UART Echo` launch configuration. The launch configuration runs these steps:

- Builds the firmware with `cmake --workflow --preset nucleo_u5a5`
- Starts Renode with `${workspaceFolder}/../renode/renode renode/debug_stm32u5a5.resc`
- Loads `build/nucleo_u5a5/build/embedded_template`
- Starts a Renode GDB server on `localhost:3333`
- Attaches `/opt/arm-gnu/bin/arm-none-eabi-gdb`
- Terminates the Renode task when the debug session ends

Renode creates the UART PTY at `/tmp/uart`.

## Customizing For A Board

Before this template can link for real hardware, add or configure the board-specific files expected by `CMakeLists.txt`:

- `linker.ld`, or pass `-DMCU_LINKER_SCRIPT=/path/to/linker.ld`
- `src/startup.s`, or pass `-DMCU_STARTUP_FILE=/path/to/startup.s`
- Device headers, system initialization, and peripheral driver sources for your selected STM32 device

Application sources are discovered recursively from `src/` for `.c`, `.cpp`, `.s`, and `.S` files.

## Useful CMake Options

- `MCU_CPU`: target CPU, for example `cortex-m0`, `cortex-m3`, `cortex-m4`, or `cortex-m7`
- `MCU_FPU`: target FPU, for example `fpv4-sp-d16`; use `auto` to skip FPU flags
- `MCU_FLOAT_ABI`: float ABI, such as `soft`, `softfp`, or `hard`
- `MCU_LINKER_SCRIPT`: linker script path
- `MCU_STARTUP_FILE`: startup assembly file path
- `PRINT_FLAGS`: set to `ON` to print compiler, assembler, and linker flags during configuration

Example with printed flags:

```sh
cmake -S . -B build -DPRINT_FLAGS=ON
```

## Testing

No automated test target is currently defined in this repository.

## Notes

- `cmake/cross.cmake` currently exists but is empty.
- The CMSIS 6 submodule is included, but the top-level build does not yet add CMSIS include paths or device-specific startup/system files automatically.
- Flashing and debugging commands are not defined yet; add tooling such as OpenOCD, pyOCD, or vendor-specific scripts as needed for your board.
