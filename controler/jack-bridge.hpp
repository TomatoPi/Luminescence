#pragma once

#include <jack/jack.h>

#include <vector>
#include <queue>
#include <cstdint>

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

  std::queue<std::vector<uint8_t>> from_jack;
  int fromjack_pipe_fd[2];

  std::queue<std::vector<uint8_t>> to_jack;
  int tojack_pipe_fd[2];
};