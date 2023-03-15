cmake_minimum_required(VERSION 3.10)
project(Controller VERSION 0.0.1)

include(FindPkgConfig)

set(HEADERS
    
  src/device/device.h
    src/device/transport/transport.h
      src/device/transport/transports/fd-proxy.h
      src/device/transport/transports/tcp.h
      src/device/transport/transports/serial.h
    src/device/transport/factory.h
  

  src/midi/bridge.h
  src/midi/bridges/jack-bridge.h
  
  src/device.h
  
  src/jack-bridge.hpp
  src/manager.hpp
  src/mapper.hpp
  )
  
  set(SOURCES
  src/controller.cpp
  
  src/device/transport/factory.cpp
  src/device/transport/transports/tcp.cpp
  src/device/transport/transports/serial.cpp
  src/device/transport/transports/fd-proxy.cpp
  
  src/midi/bridges/jack-bridge.cpp

  src/manager.cpp
  src/mapper.cpp
)

add_executable(Controller ${HEADER} ${SOURCES})
target_include_directories(Controller PUBLIC src)
target_link_libraries(Controller jack jsoncpp)
