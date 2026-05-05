cmake_minimum_required(VERSION 3.13)

if(NOT CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/cross.cmake" CACHE FILEPATH "CMake toolchain file")
endif()

project(embedded_template LANGUAGES C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EMBEDDED_TARGET "nucleo_u5a5" CACHE STRING "Embedded target to build")
set(EMBEDDED_APP "uart_echo_app" CACHE STRING "Application under src/apps to build")
set(EMBEDDED_TARGET_FILE "${CMAKE_SOURCE_DIR}/targets/${EMBEDDED_TARGET}/target.cmake")

if(NOT EXISTS "${EMBEDDED_TARGET_FILE}")
	message(FATAL_ERROR "Unknown embedded target '${EMBEDDED_TARGET}'. Expected ${EMBEDDED_TARGET_FILE}")
endif()

include("${EMBEDDED_TARGET_FILE}")

# Build type defaults
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ====== Flags ======
set(MCU_BASE_FLAGS "-mcpu=${MCU_CPU} -mthumb")
if(NOT MCU_FPU STREQUAL "auto")
	set(MCU_BASE_FLAGS "${MCU_BASE_FLAGS} -mfpu=${MCU_FPU} -mfloat-abi=${MCU_FLOAT_ABI}")
endif()

if(MCU_FAMILY STREQUAL "stm32u5")
	list(APPEND MCU_INCLUDE_DIRS
		"${CMAKE_SOURCE_DIR}/submodules/CMSIS/cmsis-device-u5/Include"
		"${CMAKE_SOURCE_DIR}/submodules/CMSIS/cmsis-core/CMSIS/Core/Include"
	)
	set(MCU_HAL_TARGET hal_stm32_u5)
else()
	message(FATAL_ERROR "Unsupported MCU family '${MCU_FAMILY}' for target '${EMBEDDED_TARGET}'")
endif()

set(COMMON_FLAGS "-ffunction-sections -fdata-sections -Wall -Wextra -fno-common")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU_BASE_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MCU_BASE_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${MCU_BASE_FLAGS}")

if(MCU_FAMILY STREQUAL "stm32u5")
	add_subdirectory("${CMAKE_SOURCE_DIR}/submodules/STM32_HAL" "${CMAKE_BINARY_DIR}/stm32_hal")
	add_subdirectory("${CMAKE_SOURCE_DIR}/src/platform" "${CMAKE_BINARY_DIR}/platform")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	string(APPEND CMAKE_C_FLAGS " -Og -g3")
else()
	string(APPEND CMAKE_C_FLAGS " -Os -g0")
endif()

# Linker flags
set(LINKER_FLAGS -Wl,--gc-sections "-Wl,-Map=${PROJECT_NAME}.map")
if(MCU_LINKER_SCRIPT)
	list(APPEND LINKER_FLAGS "-T${MCU_LINKER_SCRIPT}")
endif()

# ====== Project sources ======
set(APP_DIR "${CMAKE_SOURCE_DIR}/src/apps/${EMBEDDED_APP}")
set(APP_CMAKE_FILE "${APP_DIR}/app.cmake")

if(NOT IS_DIRECTORY "${APP_DIR}")
	message(FATAL_ERROR "Unknown embedded app '${EMBEDDED_APP}'. Expected ${APP_DIR}")
endif()

if(NOT EXISTS "${APP_CMAKE_FILE}")
	message(FATAL_ERROR "Selected embedded app '${EMBEDDED_APP}' must provide ${APP_CMAKE_FILE}")
endif()

set(APP_SRCS)
set(APP_INCLUDE_DIRS)
set(APP_COMPILE_DEFINITIONS)
set(APP_LIBS)

include("${APP_CMAKE_FILE}")

list(APPEND APP_SRCS
	"${MCU_STARTUP_FILE}"
	"${MCU_SYSTEM_FILE}"
)

if(NOT APP_SRCS)
	message(WARNING "No source files found for app '${EMBEDDED_APP}'. Add sources in ${APP_CMAKE_FILE}.")
endif()

# ====== Target ======
add_executable(${PROJECT_NAME}.elf ${APP_SRCS})

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE ${MCU_DEFINES} ${APP_COMPILE_DEFINITIONS})
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${MCU_INCLUDE_DIRS} ${APP_INCLUDE_DIRS})
target_link_options(${PROJECT_NAME}.elf PRIVATE ${LINKER_FLAGS})

target_compile_definitions(${MCU_HAL_TARGET} PUBLIC ${MCU_DEFINES})
target_include_directories(${MCU_HAL_TARGET} PUBLIC ${MCU_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}.elf PRIVATE ${MCU_HAL_TARGET} platform_stm32u5 ${APP_LIBS} m)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES
	OUTPUT_NAME ${PROJECT_NAME}
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
)

# ====== Post-build: generate .bin/.hex and show size ======
if(ARM_NONE_EABI_OBJCOPY)
	add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
		COMMAND ${ARM_NONE_EABI_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/build/${PROJECT_NAME}.bin
		COMMAND ${ARM_NONE_EABI_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${CMAKE_BINARY_DIR}/build/${PROJECT_NAME}.hex
		COMMENT "Generating .bin and .hex"
	)
endif()

if(ARM_NONE_EABI_SIZE)
	add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
		COMMAND ${ARM_NONE_EABI_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}.elf>
		COMMENT "Size of final ELF"
	)
endif()

# ====== Helpful options ======
option(PRINT_FLAGS "Print compiler flags" OFF)
if(PRINT_FLAGS)
	message(STATUS "C flags: ${CMAKE_C_FLAGS}")
	message(STATUS "CXX flags: ${CMAKE_CXX_FLAGS}")
	message(STATUS "ASM flags: ${CMAKE_ASM_FLAGS}")
	message(STATUS "Linker flags: ${LINKER_FLAGS}")
endif()

# ====== Usage hints (not executed) ======
# Configure from command line:
#  cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=path/to/arm-none-eabi.cmake -DMCU_CPU=cortex-m4 -DMCU_FPU=fpv4-sp-d16 -DMCU_FLOAT_ABI=hard
# Build:
#  cmake --build build --config Release
