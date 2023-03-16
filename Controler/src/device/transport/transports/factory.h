#pragma once

#include "../transport.h"

#include "fd-proxy.h"
#include "tcp.h"
#include "serial.h"

#include <json/json.h>

#include <memory>
#include <variant>
#include <stdexcept>
#include <type_traits>

namespace transport {
/// @brief Namespace holding methods to build proper transport
namespace factory {

  /* *** Convenient aliases typedefs *** */

  using proto_transport_signature = std::variant<
    tcp::signature, 
    serial::signature
    >;

  using transport_signature_type = std::variant<
    std::decay_t<decltype(std::declval<tcp::socket>().signature())>,
    std::decay_t<decltype(std::declval<serial::serial>().signature())>
    >;

  using transport_type = struct ::transport::transport;
  using transport_ptr     = std::unique_ptr<transport_type>;

  /* *** Exceptions *** */

  /// @brief thrown if an ill-formed json is passed to parse_signature
  struct bad_json : std::runtime_error {
    explicit bad_json(const std::string& str)
    : std::runtime_error(str)
    {}
  };

  /* *** Factory Functions *** */

  transport_signature_type parse_signature(const Json::Value& sig);

  transport_ptr open(const transport_signature_type& sig);

}
}