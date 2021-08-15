#include <jack/jack.h>
#include <jack/midiport.h>
#include <queue>
#include <array>
#include <cstring>
#include <iostream>

constexpr const size_t Msglen = 8;
std::queue<std::array<uint8_t, Msglen>> sendingqueue;
jack_port_t* outport=nullptr;

int callback(jack_nframes_t nframes, void* args)
{
  void* buffer = jack_port_get_buffer(outport, nframes);
  jack_midi_clear_buffer(buffer);
  static int i = 0;
  if (++i <= 40000)
  {
    i = 0;
    jack_midi_data_t* data = jack_midi_event_reserve(buffer, 0, Msglen);
    data[0] = 0x90; // light up top left pad
    data[1] = 0x35;
    data[2] = 0x7f;
  }
  return 0;
}

int main(int argc, const char* argv[])
{
  jack_status_t status;
  jack_client_t* client = jack_client_open("MidiSend", JackNullOption, &status);
  if (!client)
    return -1;
  
  outport = jack_port_register(client, "out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
  if (!outport)
    return -2;

  if (jack_set_process_callback(client, callback, nullptr))
    return -3;

  if (jack_activate(client))
    return -4;

  while (1)
  {
    std::array<uint8_t, Msglen> tmp;
    for (int i=0; i<Msglen; ++i)
    {
      int d = 0;
      scanf(" %d", &d);
      tmp[i] = d;
    }

    // std::cout << "Dump [";
    // for (auto x : tmp) 
    //   std::cout << (int)x << " ";
    // std::cout << '\b' << ']';

    sendingqueue.push(tmp);
  }

  jack_deactivate(client);
  jack_client_close(client);
  
  return 0;
}