#include "apc40/apc40.h"

#include "../driver/common.h"

#include <jack/jack.h>
#include <jack/midiport.h>

#include <queue>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <cmath>

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

struct {
  jack_time_t length = 0;
  jack_time_t last_hit = 0;

  uint16_t bpm() const {
    return (60lu * 1000lu * 1000lu) / (length+1);
  }
} timebase;

std::queue<std::vector<uint8_t>> serial_queue;
apc::APC40& APC40 = apc::APC40::Get();

objects::RGB optopoulpe;

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

    auto& responses = APC40.handle_midi_event((const uint8_t*)event.buffer);

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
  int arduino = serialport_init(argv[1], B115200);
  if (arduino < 0)
    return __LINE__;

  long framerate = atol(argv[2]);

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
      optopoulpe.r = static_cast<apc::Encoder*>(ctrl)->get_value();
  });
  apc::Encoder::Get(0, 1, 0)->add_routine([&](Controller::Control* ctrl){
      optopoulpe.g = static_cast<apc::Encoder*>(ctrl)->get_value();
  });
  apc::Encoder::Get(0, 2, 0)->add_routine([&](Controller::Control* ctrl){
      optopoulpe.b = static_cast<apc::Encoder*>(ctrl)->get_value();
  });

  apc::MainFader::Get()->add_routine([&](Controller::Control* ctrl){
      optopoulpe.x = static_cast<apc::MainFader*>(ctrl)->get_value();
  });

  apc::Fader::Get(0)->add_routine([&](Controller::Control* ctrl){
  });

  apc::TapTempo::Get()->add_routine([&](Controller::Control*){
    jack_time_t t = jack_get_time();
    timebase.length = t - timebase.last_hit;
    timebase.last_hit = t;
    optopoulpe.bpm = timebase.bpm();
  });

  apc::IncTempo::Get()->add_routine([&](Controller::Control* ctrl){
    optopoulpe.sync_correction = std::min((unsigned int)optopoulpe.sync_correction + 10, 255u);
    optopoulpe.bpm = timebase.bpm();
  });
  apc::DecTempo::Get()->add_routine([&](Controller::Control* ctrl){
    optopoulpe.sync_correction = std::max((unsigned int)optopoulpe.sync_correction + 10, 0u);
    optopoulpe.bpm = timebase.bpm();
  });

  apc::SyncPot::Get()->add_routine([&](Controller::Control* ctrl){
    uint8_t val = static_cast<apc::SyncPot*>(ctrl)->get_value();
    if (val < 64)
      timebase.length *= 0.95;
    else
      timebase.length *= 1.05;
    optopoulpe.bpm = timebase.bpm();
  });

  Serializer serializer;

  // APC40.dump();

  while (1)
  {
    char buffer[512];
    int res = 0;
    int timeout_cptr = 0;
    bool received_start = false;
    do
    {
      res = serialport_read_until(arduino, buffer, STOP_BYTE, 512, framerate);
      if (res == -2)
      {
        ++timeout_cptr;
        if (10 < timeout_cptr)
          break;
        continue;
      }
      if (static_cast<uint8_t>(*buffer) == static_cast<uint8_t>(STOP_BYTE))
      {
        // fprintf(stderr, "RCV : %d : START\n", res);
        received_start = true;
      }
      else if (*buffer && res != -1)
      {
        fprintf(stderr, "RCV : %d : %s\n", res, buffer);
      }
      else
        break;
    }
    while (1);

    APC40.update_dirty_controls();
    if (received_start)
    {
      received_start = false;
      const uint8_t* raw = serializer.serialize(optopoulpe);
      for (size_t i = 0 ; i < SerialPacket::Size ; ++i)
      {
        uint8_t byte = raw[i];
        serialport_writebyte(arduino, byte);
        // fprintf(stderr, "0x%02x ", byte); 
      }
      // fprintf(stderr, "\n");
    }

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
    // usleep(100000 / framerate);
  }
  
  return 0;
}