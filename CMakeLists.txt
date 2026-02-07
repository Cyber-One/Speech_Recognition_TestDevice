cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK
include(pico_sdk_import.cmake)

project(I2C_TestDevice C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Create executable
add_executable(I2C_TestDevice
    i2c_test_device.c
    st7796_driver.c
)

# Pull in common dependencies
target_link_libraries(I2C_TestDevice
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_spi
    pico_multicore
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(I2C_TestDevice 1)
pico_enable_stdio_uart(I2C_TestDevice 0)

# Create UF2 file for main target
pico_add_extra_outputs(I2C_TestDevice)

# Minimal test executable (for troubleshooting)
add_executable(test_minimal
    test_minimal.c
)

target_link_libraries(test_minimal
    pico_stdlib
    hardware_gpio
)

pico_enable_stdio_usb(test_minimal 1)
pico_enable_stdio_uart(test_minimal 0)

# Create UF2 file for minimal test
pico_add_extra_outputs(test_minimal)
