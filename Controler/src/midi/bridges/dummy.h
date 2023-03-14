#pragma once

#include "midi/bridge.h"

namespace midi {

  class dummy_bridge : public bridge {
  public :

    void activate() override
    {}

    std::vector<raw_message> incomming_midi() override
    { return std::vector<raw_message>(); }

    void send_midi(const raw_message& msg) override
    {}
  };

}