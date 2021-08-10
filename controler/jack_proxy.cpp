#include <jack/jack.h>
#include <jack/midiport.h>

#include <queue>
#include <stdio.h>
#include <unistd.h>

#include "arduino-serial-lib.h"
#include <termios.h>

const char* pgm_name = "DMX-Duino-Proxy";

jack_client_t* client;
jack_port_t* midi_in;

std::queue<std::vector<jack_midi_data_t>> midi_queue;

struct Status
{

};

int jack_callback(jack_nframes_t nframes, void* args)
{
  Status& status = *static_cast<Status*>(args);

  jack_nframes_t events_count;
  jack_midi_event_t event;

  void* buffer = jack_port_get_buffer(midi_in, nframes);
  events_count = jack_midi_get_event_count(buffer);

  for (jack_nframes_t i = 0 ; i < events_count ; ++i)
  {
    if (0 != jack_midi_event_get(&event, buffer, i))
      continue;

    std::vector<jack_midi_data_t> tmp;
    for (size_t i = 0 ; i < event.size ; ++i)
      tmp.emplace_back(event.buffer[i]);
    
    midi_queue.emplace(tmp);
  }

  return 0;
}

int main(int argc, const char* argv[])
{
  int arduino = serialport_init(argv[1], B9600);
  if (arduino < 0)
    return -100;

  jack_status_t jack_status;
  client = jack_client_open(pgm_name, JackNullOption, &jack_status);
  if (nullptr == client)
    return -1;

  midi_in = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (nullptr == midi_in)
    return -2;

  Status pgm_status;
  if (0 != jack_set_process_callback(client, jack_callback, &pgm_status))
    return -4;

  if (0 != jack_activate(client))
    return -5;

  while (1)
  {
    while (!midi_queue.empty())
    {
      const auto& event = midi_queue.back();
      for (const auto& byte : event)
      {
        serialport_writebyte(arduino, byte);
        fprintf(stderr, "0x%02x ", byte);
      }
      fprintf(stderr, "\n");
      midi_queue.pop();
    }
    usleep(10);
  }
  
  return 0;
}