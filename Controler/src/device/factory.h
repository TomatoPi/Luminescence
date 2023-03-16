#pragma once

#include "device.h"
#include "transport/transports/factory.h"

#include <json/json.h>

#include <variant>
#include <future>
#include <optional>
#include <queue>

namespace opto {
namespace device {

  using pending_device = std::future<class device>;
  using maybe_device = std::variant<class device, pending_device, signature>;
  using opt_device = std::optional<class device>;

  class factory {
  public :
    


    void push(const signature& sig) = 0;
    opt_device pull() = 0;

  };

}
}