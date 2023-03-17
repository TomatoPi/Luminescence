#pragma once

#include "transport/transport.h"

#include "transport/transports/factory.h"

#include <tuple>
#include <utility>
#include <variant>
#include <memory>
#include <stdexcept>
#include <future>

#include <iostream>

/// @brief Main namespace for optopoulpe top objects
namespace opto {
namespace device {

  namespace meta {
    
    struct name { std::string value = "default-device"; };
    using transport = ::transport::factory::transport_signature_type;
  }

  using signature = std::tuple<meta::name, meta::transport>;
  
  class device {
  public :

    using transport_ptr = ::transport::factory::transport_ptr;

    device() = default;
    ~device() = default;

    device(const device&) = delete;
    device& operator= (const device&) = delete;

    device(device&& d) : device(std::move(d._cfg), std::move(d._transport)) { d._transport = nullptr; }
    device& operator= (device&& s)
    { 
      _cfg = std::move(s._cfg);
      _transport = std::move(s._transport);
      s._transport = nullptr;
      return *this;
    }

    explicit device(const signature& sig)
    : device(sig, transport::factory::open(std::get<meta::transport>(sig)))
    {}

    explicit operator signature () const noexcept
    { return _cfg; }

    bool alive() noexcept
    { return _transport && _transport->alive(); }

    void kill()
    { _transport = nullptr; }

    transport::packet_status send(const transport::packet_payload& p)
    { return _transport->send(p); }

    transport::opt_reply receive()
    { return _transport->receive(); }

  private :

    device(const signature& sig, transport_ptr&& tr)
    : _cfg(sig), _transport(std::forward<decltype(tr)>(tr))
    {}

    // static maybe_transport open(const meta::transport& sig);

    signature     _cfg;
    transport_ptr _transport;
  };

}
}