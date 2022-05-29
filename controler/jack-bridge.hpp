#pragma once

#include <jack/jack.h>

#include <vector>
#include <queue>
#include <cstdint>

#include "thread-queue.hpp"

class JackBridge {
public :
  JackBridge(const char* name);
  ~JackBridge();

  void activate();

  std::vector<std::vector<uint8_t>> incomming_midi();
  void send_midi(std::vector<uint8_t>&& msg);

private :
  friend int jack_callback(jack_nframes_t nframes, void* args);

  jack_client_t* client = nullptr;
  jack_port_t* midi_in = nullptr;
  jack_port_t* midi_out = nullptr;

  ThreadSafeQueue<std::vector<uint8_t>> from_jack, to_jack;
};