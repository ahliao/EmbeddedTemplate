add_library(platform_stm32u5 OBJECT
    stm32u5/platform_uart_stm32u5.c
)

target_include_directories(platform_stm32u5
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/common
        ${CMAKE_CURRENT_LIST_DIR}/stm32u5
)

if(TARGET hal_stm32_u5)
    target_link_libraries(platform_stm32u5 PUBLIC hal_stm32_u5)
endif()

if(DEFINED MCU_DEFINES)
    target_compile_definitions(platform_stm32u5 PUBLIC ${MCU_DEFINES})
endif()
