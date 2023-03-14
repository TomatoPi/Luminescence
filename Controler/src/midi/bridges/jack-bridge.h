#pragma once
#ifdef __unix__

#include <jack/jack.h>

#include <vector>
#include <queue>
#include <cstdint>

#include "thread-queue.hpp"
#include "midi/bridge.h"

namespace midi::jack {

  class JackBridge : public midi::bridge {
  public :
    explicit JackBridge(const char* name);
    ~JackBridge() noexcept;

    void activate() override;

    std::vector<raw_message> incomming_midi() override;
    void send_midi(const raw_message& msg);

  private :
    friend int jack_callback(jack_nframes_t nframes, void* args);

    jack_client_t* client = nullptr;
    jack_port_t* midi_in = nullptr;
    jack_port_t* midi_out = nullptr;

    ThreadSafeQueue<std::vector<uint8_t>> from_jack, to_jack;
  };
}

#endif // #ifdef __unix__
