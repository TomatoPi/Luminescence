#include "apc40/apc40.h"

#include "driver/frame.h"

#include <jack/jack.h>
#include <jack/midiport.h>

#include <queue>
#include <stdio.h>
#include <unistd.h>
#include <cstring>

#include "arduino-serial-lib.h"
#include <termios.h>

/*
  Pads : NoteOn 0x90, NoteOff 0x80
    channel : colonne : 0 - 8
    d1 : ligne : 0x35 - 0x39
                 0x34 ligne du bas

    dernière colonne : 0x52 - 0x56
                       0x51 bas droite

    momentanné

  Selection track : CC 0xb0
    channel : colonne
    d1 : 0x17
    8 messages d'affilé.

    stationnaire

  Activator :
    NoteOn, NoteOff
    channel : colonne
    note :
      0x32 : Activator
      0x31 : Solo
      0x30 : Arm

  Faders :
    CC, channel : colonne, 0x07, value

  Master : 
    0xb0 0x0e value

  Encodeurs :
    0xb0 0x30-0x33 value
    0xb0 0x34-0x37 value

    0xb0 0x10-0x13 value
    0xb0 0x14-0x17 value

  Play, Stop, Rec :
    NoteOn,NoteOff
    0x5b,c,d

  TapTempo :
    Note, 0x63
  
  Nudge +, - :
    Note, 0x65 0x64
*/

const char* pgm_name = "DMX-Duino-Proxy";

jack_client_t* client;
jack_port_t* midi_in;
jack_port_t* midi_out;

std::queue<std::vector<uint8_t>> serial_queue;
apc::APC40 APC40;
Optopoulpe optopoulpe;

int jack_callback(jack_nframes_t nframes, void* args)
{
  (void) args;

  jack_nframes_t events_count;
  jack_midi_event_t event;

  void* out_buffer = jack_port_get_buffer(midi_out, nframes);
  jack_midi_clear_buffer(out_buffer);

  void* in_buffer = jack_port_get_buffer(midi_in, nframes);
  events_count = jack_midi_get_event_count(in_buffer);

  for (jack_nframes_t i = 0 ; i < events_count ; ++i)
  {
    if (0 != jack_midi_event_get(&event, in_buffer, i))
      continue;

    if (3 != event.size)
      continue;

    for (size_t i = 0 ; i < 3 ; ++i)
    {
      fprintf(stderr, "0x%02x ", event.buffer[i]);
    }
    fprintf(stderr, "\n");

    auto& responses = APC40.handle_midi_event(event.buffer);

    for (auto& msg : responses)
    {
      auto tmp = msg.serialize();
      void* raw = jack_midi_event_reserve(out_buffer, event.time, tmp.size());
      memcpy(raw, tmp.data(), tmp.size());
    }
  }

  return 0;
}

int main(int argc, const char* argv[])
{
  int arduino = serialport_init(argv[1], B921600);
  if (arduino < 0)
    return __LINE__;

  jack_status_t jack_status;
  client = jack_client_open(pgm_name, JackNullOption, &jack_status);
  if (nullptr == client)
    return __LINE__;

  midi_in = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (nullptr == midi_in)
    return __LINE__;

  midi_out = jack_port_register(client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
  if (nullptr == midi_in)
    return __LINE__;

  if (0 != jack_set_process_callback(client, jack_callback, nullptr))
    return __LINE__;

  if (0 != jack_activate(client))
    return __LINE__;

  const char** ports_names = jack_get_ports(client, nullptr, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
  if (nullptr == ports_names)
    return __LINE__;
  
  if (0 != jack_connect(client, ports_names[0], "DMX-Duino-Proxy:midi_in"))
    return __LINE__;

  jack_free(ports_names);


  ports_names = jack_get_ports(client, nullptr, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
  if (nullptr == ports_names)
    return __LINE__;
  
  if (0 != jack_connect(client, "DMX-Duino-Proxy:midi_out", ports_names[0]))
    return __LINE__;

  jack_free(ports_names);

  apc::Encoder::Get(0, 0, 0)->add_routine([&](Controller::Control* ctrl){
    for (auto& c : optopoulpe.frame)
      c.r = static_cast<apc::Encoder*>(ctrl)->get_value();
  });
  apc::Encoder::Get(0, 1, 0)->add_routine([&](Controller::Control* ctrl){
    for (auto& c : optopoulpe.frame)
      c.g = static_cast<apc::Encoder*>(ctrl)->get_value();
  });
  apc::Encoder::Get(0, 2, 0)->add_routine([&](Controller::Control* ctrl){
    for (auto& c : optopoulpe.frame)
      c.b = static_cast<apc::Encoder*>(ctrl)->get_value();
  });

  while (1)
  {
    APC40.update_dirty_controls();
    serialport_writebyte(arduino, 15);
    serialport_write(arduino, reinterpret_cast<const char*>(optopoulpe.frame[0].raw));
    // while (!serial_queue.empty())
    // {
    //   const auto& event = serial_queue.front();
    //   for (const auto& byte : event)
    //   {
    //     fprintf(stderr, "0x%02x ", byte);
    //   }
    //   fprintf(stderr, "\n");
    //   serial_queue.pop();
    // }
    usleep(1000 / 25);
  }
  
  return 0;
}