cmake_minimum_required(VERSION 3.10)
project(Controller VERSION 0.0.1)

include(FindPkgConfig)

set(HEADERS
  thread-queue.hpp
  midi-device.hpp
  mapper.hpp
  manager.hpp
  jack-bridge.hpp
  arduino-bridge.hpp
)

set(SOURCES
  mapper.cpp
  manager.cpp
  jack-bridge.cpp
  controller.cpp
  arduino-bridge.cpp
)

add_executable(Controller ${HEADER} ${SOURCES})

target_link_libraries(Controller jack)
