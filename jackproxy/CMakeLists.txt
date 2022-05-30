cmake_minimum_required(VERSION 3.10)
project(TCP-Bridge VERSION 0.0.1)

include(FindPkgConfig)

set(HEADERS
  arduino-serial-lib.h
)

set(SOURCES
  arduino-bridge.c
  arduino-serial-lib.c
)

add_executable(TCP-Bridge ${HEADER} ${SOURCES})
