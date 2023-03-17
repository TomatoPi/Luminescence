#pragma once

#include "../transport.h"

#include <iostream>
#include <tuple>

namespace transport {

  class dummy : public transport::transport {
  public:

    using signature_type = std::tuple<>;

    explicit dummy(const signature_type&) {};

    dummy() = default;

    dummy(const dummy&) = default;
    dummy& operator= (const dummy&) = default;

    dummy(dummy&&) = default;
    dummy& operator= (dummy&&) = default;

    virtual ~dummy() noexcept = default;

    bool alive() const noexcept override
    { return true; }

  protected :

    packet_status vsend(const packet_payload& p) override
    {
      std::cout << "Send : " << p.size() << "bytes\n";
      return packet_status::Sent;
    }
    opt_reply vreceive() override
    {
      return std::make_optional(reply{"Received something"});
    }
  };

}