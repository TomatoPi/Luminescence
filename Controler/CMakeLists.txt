cmake_minimum_required(VERSION 3.10)
project(Controller VERSION 0.0.1)

include(FindPkgConfig)

set(HEADERS
  src/transport/transport.h
  src/transport/transports/tcp.h
  src/transport/transports/serial.h

  src/midi/bridge.h
  src/midi/bridges/jack-bridge.h
  
  src/device.h
  
  src/jack-bridge.hpp
  src/manager.hpp
  src/mapper.hpp
  )
  
  set(SOURCES
  src/controller.cpp
  
  src/transport/transports/tcp.cpp
  src/transport/transports/serial.cpp
  
  src/midi/bridges/jack-bridge.cpp

  src/manager.cpp
  src/mapper.cpp
)

add_executable(Controller ${HEADER} ${SOURCES})
target_include_directories(Controller PUBLIC src)
target_link_libraries(Controller jack jsoncpp)
