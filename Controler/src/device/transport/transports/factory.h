#pragma once

#include "../transport.h"

#include "fd-proxy.h"
#include "tcp.h"
#include "serial.h"

#include <json/json.h>

#include <memory>
#include <future>
#include <variant>
#include <stdexcept>

#include <iostream>

namespace transport {
/// @brief Namespace holding methods to build proper transport
namespace factory {

  /* *** Convenient aliases typedefs *** */

  using transport_type = struct ::transport::transport;

  using transport_ptr     = std::unique_ptr<transport_type>;
  using pending_transport = std::future<transport_ptr>;

  using transport_signature_type = std::variant<
    tcp::signature,
    serial::signature
    >;

  /* *** Exceptions *** */

  struct bad_json { std::string what; };

  /* *** Factory Functions *** */

  transport_signature_type parse_signature(const Json::Value& sig);
  pending_transport open(const transport_signature_type& sig);

}
}