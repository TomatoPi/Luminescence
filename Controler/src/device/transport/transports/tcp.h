#pragma once

#include "fd-proxy.h"

#include <string>
#include <tuple>

namespace transport {
/// @brief Namespace holding TCP transport related objects
namespace tcp {

  /* *** TCP Transport Properties *** */

  /// @brief Represents an IPv4 address using dotted notation
  struct address {
    std::string host;
    std::string port;
  };

  /// @brief Holds configuration for the linux socket
  struct keepalive_config {
    int counter = 100;
    int interval = 1;
  };

  /// @brief Holds all parameters required to build a valid socket
  using signature = std::tuple<address, keepalive_config>;

  /* *** TCP transport implementation *** */

  /// @brief Try to open a linux tcp non blocking connection to given host
  /// @param addr address of the server to connect to
  /// @return opened socket's file descriptor on success, throw on failure
  int open_socket(const std::tuple<address, keepalive_config, fd::read_buffer_size>& sig);

  /// @brief Close the holded file descriptor if exists, throw on failure
  void close_socket(int fd);

  using socket = fd::proxy<
    signature, 
    decltype(open_socket), &open_socket,
    decltype(close_socket), &close_socket
    >;
}
}