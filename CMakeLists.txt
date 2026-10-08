cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_st7735 C CXX ASM)
pico_sdk_init()

add_library(st7735 STATIC
    src/st7735.c
    src/font5x7.c
)
target_include_directories(st7735 PUBLIC src)
target_link_libraries(st7735 PUBLIC pico_stdlib hardware_spi hardware_gpio)

add_subdirectory(examples/demo)
add_subdirectory(examples/benchmark)