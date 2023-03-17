cmake_minimum_required(VERSION 3.10)
project(Controller VERSION 0.0.1)

include(FindPkgConfig)

set(DEVICE_HEADERS
    
  src/device/device.h
  src/device/factory.h
    src/device/transport/transport.h
      src/device/transport/transports/fd-proxy.h
      src/device/transport/transports/tcp.h
      src/device/transport/transports/serial.h
      src/device/transport/transports/factory.h
  

  
  )
  
  set(DEVICE_SOURCES
  
  
  src/device/device.cpp
  src/device/factory.cpp
    src/device/transport/transports/tcp.cpp
    src/device/transport/transports/serial.cpp
    src/device/transport/transports/fd-proxy.cpp
    src/device/transport/transports/factory.cpp
    )
    
add_executable(Controller
    ${DEVICE_HEADERS} ${DEVICE_SOURCES}
    src/controller.cpp
    
  src/midi/bridge.h
  src/midi/bridges/jack-bridge.h
    src/midi/bridges/jack-bridge.cpp
    src/midi/bridges/jack-bridge.h
    src/manager.hpp

    src/manager.cpp

    src/mapper.hpp
    src/mapper.cpp
  )
target_include_directories(Controller PUBLIC src)
target_link_libraries(Controller jack jsoncpp)


add_executable(JsonCtrl)

target_sources(JsonCtrl
  PRIVATE ${DEVICE_HEADERS} ${DEVICE_SOURCES}
  PRIVATE src/exec/json-ctrl.cpp
  )
target_include_directories(JsonCtrl PUBLIC src)
target_link_libraries(JsonCtrl jsoncpp)