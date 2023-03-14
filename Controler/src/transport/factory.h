#pragma once

#include "transport.h"

#include "utils/visitor.h"

#include <json/json.h>

#include <memory>
#include <future>
#include <variant>
#include <stdexcept>

#include <iostream>

namespace transport {
/// @brief Namespace holding methods to build proper transport
namespace factory {

  using transport_type = struct ::transport::transport;

  using transport_ptr     = std::unique_ptr<transport_type>;
  using pending_transport = std::future<transport_ptr>;
  using maybe_transport   = std::variant<transport_ptr, pending_transport>;



  template <typename Tr>
  maybe_transport open(const json::Value& sig)
  {
    return std::async(std::launch::async, [](const Sig& s) -> transport_ptr {
      return std::make_unique<Tr>(s);
    }, sig);
  }
}
}