#pragma once

#include "transport.h"

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

  /* *** Exceptions *** */

  struct bad_json { std::string what; };

  /* *** Factory Functions *** */

  pending_transport open(const Json::Value& sig);
}
}