#pragma once

#include "device.h"
#include "transport/transport/factory.h"

#include <json/json.h>

#include <variant>

namespace opto {
namespace device {

  class factory {
  public :

    using transport_ptr = transport::factory::transport_ptr;
    using pending_transport = transport::factory::pending_transport;

    using maybe_device = std::variant<
      device::device,
      
      >;

  private :
    
  };

}
}