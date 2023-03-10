#pragma once

#include "transport/transport.h"
#include "transport/transports/tcp.h"
#include "transport/transports/serial.h"

#include "utils/visitor.h"

#include <variant>
#include <memory>

/// @brief Main namespace for optopoulpe top objects
namespace opto {

  using transport_signature_type = std::variant<
    tcp::socket_signature,
    serial::serial_signature
    >;

  class device {
  public :



  private :

    auto open(const transport_signature_type& sig)
    try {
      return std::visit(utils::visitor{
        [](const tcp::socket_signature& s)
          -> std::unique_ptr<transport::transport>
          { return std::make_unique<tcp::socket>(s); }
          ,
        [](const serial::serial_signature& s)
          -> std::unique_ptr<transport::transport>
          { return std::make_unique<serial::serial>(s); }
      }, sig);
    }

    transport_signature_type    _sig;
    std::unique_ptr<transport::transport>  _transport;
  };

}