cmake_minimum_required(VERSION 3.10)
project(Driver VERSION 0.0.1)

add_executable(Driver driver.ino)

target_link_arduino_libraries(Driver PRIVATE core FastLED)
target_enable_arduino_upload(Driver)