set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/cmake/ArduinoToolchain.cmake)

cmake_minimum_required(VERSION 3.10)
project(Driver VERSION 0.0.1)

set(ARDUINO_DEFAULT_BOARD due) # Default Board ID
set(ARDUINO_DEFAULT_PORT /dev/ttyACM0) # Default Port

generate_arduino_firmware(driver SRCS driver.ino)