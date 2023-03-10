cmake_minimum_required(VERSION 3.10)
project(Controller VERSION 0.0.1)

include(FindPkgConfig)

set(HEADERS
  src/transport/transport.h
  src/transport/transports/tcp.h
  src/transport/transports/serial.h
  )
  
  set(SOURCES
  src/main.cpp
  
  src/transport/transports/tcp.cpp
  src/transport/transports/serial.cpp
)

add_executable(Controller ${HEADER} ${SOURCES})
target_include_directories(Controller PUBLIC src)
target_link_libraries(Controller jack)
