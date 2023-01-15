cmake_minimum_required(VERSION 3.24.1)

include(pico_sdk_import.cmake)

project(picostation)

pico_sdk_init()

add_executable(picostation)

target_sources(picostation PRIVATE main.c utils.c subq.c cmd.c hw_config.c i2s.c)

add_subdirectory(FatFs_SPI build)

pico_enable_stdio_usb(picostation 1)

pico_generate_pio_header(picostation ${CMAKE_CURRENT_LIST_DIR}/main.pio)

target_link_libraries(picostation PRIVATE pico_stdlib hardware_pio hardware_dma FatFs_SPI pico_multicore)

pico_set_binary_type(picostation copy_to_ram)

pico_add_extra_outputs(picostation)
