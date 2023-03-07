cmake_minimum_required(VERSION 3.10)
project(Controller VERSION 0.0.1)

include(FindPkgConfig)

set(HEADERS
  src/bridge.h
  src/bridges/tcp.h
  src/utils/lock.h
  )
  
set(SOURCES
  src/bridge.cpp
  src/bridges/tcp.cpp
  src/controller.cpp
)

add_executable(Controller ${HEADER} ${SOURCES})
target_include_directories(Controller PUBLIC src)
target_link_libraries(Controller jack)
