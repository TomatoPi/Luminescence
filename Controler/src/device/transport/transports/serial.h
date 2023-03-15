#pragma once

#include "fd-proxy.h"

#include <string>
#include <tuple>

namespace transport {
/// @brief Namespace holding Serial transport related objects
namespace serial {

  /* *** Serial Transport Properties *** */

  /// @brief Represents a serial port like /dev/ttyACM0 with an associated baudrate
  struct address {
    std::string port;
    int baudrate;
  };

  /// @brief Holds all parameters required to build a valid connection
  using signature = std::tuple<address, fd::read_buffer_size>;

  /* *** Serial transport implementation *** */

  /// @brief takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
  ///   and a baud rate (bps) and connects to that port at that speed and 8N1.
  ///   opens the port in fully raw mode so you can send binary data.
  /// @return a valid file descriptor, throws on failure
  int open_serial(const signature& addr);

  /// @brief Close the holded file descriptor if exists, throw on failure
  void close_serial(int fd);

  using serial = fd::proxy<
    signature,
    decltype(open_serial), &open_serial,
    decltype(close_serial), &close_serial
    >;
}
}