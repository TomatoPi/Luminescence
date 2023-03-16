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
    
    struct name { std::string value; };
    using transport = transport::factory::transport_signature_type;
  }

  using signature = std::tuple<meta::name, meta::transport>;
  
  class device {
  public :

    using transport_ptr = transport::factory::transport_ptr;
    using pending_transport = transport::factory::pending_transport;

    using maybe_transport = std::variant<transport_ptr, pending_transport>;

    device() = default;
    ~device() = default;

    device(const device&) = delete;
    device& operator= (const device&) = delete;

    device(device&&) = default;
    device& operator= (device&&) = default;

    explicit device(const signature& sig)
    : device(sig, transport::factory::open(std::get<meta::transport>(sig)))
    {}

    explicit operator signature () const noexcept
    { return _cfg; }

    bool alive() noexcept
    { return is_available(); }

    void kill()
    { _transport = nullptr; }


    bool is_available() const noexcept;
    bool make_available();

    transport::packet_status send(const transport::packet_payload& p);

    transport::opt_reply receive();

  private :

    device(const signature& sig, maybe_transport&& tr)
    : _cfg(sig), _transport(std::forward<decltype(tr)>(tr))
    {}

    static maybe_transport open(const meta::transport& sig);

    signature       _cfg;
    maybe_transport _transport;
  };

}
}