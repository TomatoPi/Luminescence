#pragma once

#include "../transport.h"

#include "fd-proxy.h"
#include "tcp.h"
#include "serial.h"

#include <json/json.h>

#include <memory>
#include <variant>
#include <type_traits>

namespace transport {
/// @brief Namespace holding methods to build proper transport
namespace factory {

  /* *** Convenient aliases typedefs *** */

  using proto_transport_signature = std::variant<tcp::signature, serial::signature>;

  using tcp_signature = std::decay_t<decltype(std::declval<tcp::socket>().signature())>;
  using serial_signature = std::decay_t<decltype(std::declval<serial::serial>().signature())>;

  using transport_signature_type = std::variant<tcp_signature, serial_signature>;

  using transport_type = struct ::transport::transport;
  using transport_ptr     = std::unique_ptr<transport_type>;



  /* *** Factory Functions *** */

  transport_signature_type parse_signature(const Json::Value& root);

  Json::Value make_json(const transport_signature_type& sig);


  transport_ptr open(const transport_signature_type& sig);

}
}