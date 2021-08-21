#include "apc40/apc40.h"

#include "arduino.h"

#include "../driver/common.h"

#include <jack/jack.h>
#include <jack/midiport.h>
#include <cstring>

#include <cmath>

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

  PAN,SendA - C :
    Note, 0x57 - 0x5a 
*/

const char* pgm_name = "DMX-Duino-Proxy";

jack_client_t* client;
jack_port_t* midi_in;
jack_port_t* midi_out;

bool dirty_controller = false;

const char* save_file = nullptr;

struct Environement
{
  objects::Setup setup;
  objects::Master master;
  std::array<objects::Compo, apc::PadsColumnsCount> compos;
  std::array<objects::Oscilator, 3> oscilators;
  objects::Sequencer sequencer;
} Global;

void update_controller_internals()
{
  apc::PadsMaster::Get(3)->set_status(Global.master.blur);
  apc::PadsMaster::Get(4)->set_status(Global.master.fade);

  apc::SequencerPads::Generate([&](uint8_t col, uint8_t row){
    apc::SequencerPads::Get(col, row)->set_status(!!(Global.sequencer.steps[row] & (1 << col)));
  });
  
  apc::StrobeParams::Get(0)->set_value((32 - Global.master.strobe_speed) << 3);
  apc::StrobeParams::Get(1)->set_value((Global.master.istimemod) << 7);
  apc::StrobeParams::Get(2)->set_value((Global.master.pulse_width) << 6);
  apc::StrobeParams::Get(3)->set_value((Global.master.feedback) << 1);
  apc::StrobeEnable::Get()->set_status(Global.master.do_strobe);

  for (size_t osc = 0 ; osc < 3 ; ++osc)
  {
    apc::OscParams::Get(osc, 0)->set_value(Global.oscilators[osc].kind << 4);
    apc::OscParams::Get(osc, 1)->set_value(Global.oscilators[osc].param1 << 1);
    apc::OscParams::Get(osc, 2)->set_value(Global.oscilators[osc].subdivide << 5);
    apc::OscParams::Get(osc, 3)->set_value(Global.oscilators[osc].source << 6);
  }

  for (size_t bank = 0 ; bank < apc::TracksCount ; ++bank)
  {
    apc::PadsMatrix::Get(bank, 0)->set_status(Global.compos[bank].map_on_index);
    apc::PadsMatrix::Get(bank, 1)->set_status(Global.compos[bank].effect1);
    apc::PadsMatrix::Get(bank, 2)->set_status(Global.compos[bank].blend_mask);
    apc::PadsMatrix::Get(bank, 3)->set_status(Global.compos[bank].stars);
    apc::PadsMatrix::Get(bank, 4)->set_status(Global.compos[bank].strobe);
    apc::PadsMatrix::Get(bank, 5)->set_status(Global.compos[bank].trigger);

    apc::BottomEncoders::Get(bank, 0, 0)->set_value(Global.compos[bank].palette << 4);
    apc::BottomEncoders::Get(bank, 0, 1)->set_value(Global.compos[bank].palette_width << 1);
    apc::BottomEncoders::Get(bank, 1, 0)->set_value(Global.compos[bank].mod_intensity << 1);
    apc::BottomEncoders::Get(bank, 1, 1)->set_value(Global.compos[bank].speed << 6);
    apc::BottomEncoders::Get(bank, 2, 0)->set_value(Global.compos[bank].index_offset << 1);
    apc::BottomEncoders::Get(bank, 2, 1)->set_value(Global.compos[bank].param1 << 1);
    apc::BottomEncoders::Get(bank, 3, 0)->set_value(Global.compos[bank].blend_overlay << 1);
    apc::BottomEncoders::Get(bank, 3, 1)->set_value(Global.compos[bank].param_stars << 1);
  }

  apc::MainFader::Get()->set_value(Global.master.brightness);
}

int load()
{
  FILE* file = fopen(save_file, "rb");
  if (nullptr == file)
    { perror(""); return __LINE__; }

  size_t size;
  SerialPacket result;
  while (0 != (size = fread(&result, sizeof(result), 1, file)))
  {
    fprintf(stderr, "Read %ld bytes\n", size);
    switch (result.flags)
    {
      case objects::flags::Setup:
      {
        Global.setup = result.read<objects::Setup>();
        break;
      }
      case objects::flags::Master:
        Global.master = result.read<objects::Master>();
        break;
      case objects::flags::Composition:
      {
        objects::Compo tmp = result.read<objects::Compo>();
        Global.compos[tmp.index] = tmp;
        fprintf(stderr, "Compo : %d %d %d\n", tmp.index, tmp.palette, tmp.palette_width);
        break;
      }
      case objects::flags::Oscilator:
      {
        auto tmp = result.read<objects::Oscilator>();
        Global.oscilators[tmp.index] = tmp;
        break;
      }
      case objects::flags::Sequencer:
      {
        Global.sequencer = result.read<objects::Sequencer>();
        break;
      }
    }
  }
  fclose(file);
  update_controller_internals();
  dirty_controller = true;
  return 0;
}

int save()
{
  FILE* file = fopen(save_file, "wb");
  if (nullptr == file)
    { perror(""); return __LINE__; }
  
  auto save = [file] (auto& obj) {
    auto packet = Serializer::serialize(obj);
    auto stream = Serializer::bytestream(packet);
    fwrite(stream, sizeof(packet), 1, file);
  };

  save(Global.setup);
  save(Global.master);
  for (auto& compo : Global.compos)
    save(compo);
  for (auto& osc : Global.oscilators)
    save(osc);
  save(Global.sequencer);

  fclose(file);
  return 0;
}

struct {
  jack_time_t length = 0;
  jack_time_t last_hit = 0;

  uint16_t bpm() const {
    return (60lu * 1000lu * 1000lu) / (length+1);
  }
} timebase;

apc::APC40& APC40 = apc::APC40::Get();
std::array<std::pair<Arduino*, objects::Setup>, 2> arduinos;

template <typename T>
void push(const T& obj, uint8_t flags = 0)
{
  for (auto& [arduino, _] : arduinos)
  {
    arduino->push(obj, flags);
  }
}

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

    if (dirty_controller)
    {
      dirty_controller = false;
      auto& responces = APC40.refresh_all_controllers(); 
      for (auto& msg : responces)
      {
        auto tmp = msg.serialize();
        void* raw = jack_midi_event_reserve(out_buffer, event.time, tmp.size());
        memcpy(raw, tmp.data(), tmp.size());
      }
    }
  }

  return 0;
}

int main(int argc, const char* argv[])
{
  
  objects::Setup& setup = Global.setup;
  objects::Master& master = Global.master;
  std::array<objects::Compo, apc::PadsColumnsCount>& compos = Global.compos;
  std::array<objects::Oscilator, 3>& oscilators = Global.oscilators;
  objects::Sequencer& sequencer = Global.sequencer;

  uint8_t idx = 0;
  for (auto& arduino : arduinos)
  {
    arduino = std::make_pair(new Arduino(argv[idx+2], idx), objects::Setup{
      idx
    });
    ++idx;
  }

  jack_status_t jack_status;
  client = jack_client_open(pgm_name, JackNullOption, &jack_status);
  if (nullptr == client)
    { perror(""); return __LINE__; }

  midi_in = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  if (nullptr == midi_in)
    { perror(""); return __LINE__; }

  midi_out = jack_port_register(client, "midi_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
  if (nullptr == midi_in)
    { perror(""); return __LINE__; }

  if (0 != jack_set_process_callback(client, jack_callback, nullptr))
    { perror(""); return __LINE__; }

  if (0 != jack_activate(client))
    { perror(""); return __LINE__; }

  const char** ports_names = jack_get_ports(client, nullptr, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
  if (nullptr == ports_names)
    { perror(""); return __LINE__; }
  
  if (0 != jack_connect(client, ports_names[0], "DMX-Duino-Proxy:midi_in"))
    { perror(""); return __LINE__; }

  jack_free(ports_names);


  ports_names = jack_get_ports(client, nullptr, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
  if (nullptr == ports_names)
    { perror(""); return __LINE__; }
  
  if (0 != jack_connect(client, "DMX-Duino-Proxy:midi_out", ports_names[0]))
    { perror(""); return __LINE__; }

  jack_free(ports_names);


  apc::Save::Get()->add_routine([](auto){
    if (int err = save())
      perror("Warning Save Failure");
  });
  apc::Load::Get()->add_routine([](auto){
    if (int err = load())
      perror("Warning Load Failure");
  });

  apc::PadsMaster::Get(3)->add_routine([&master](Controller::Control* ctrl){
      master.blur = static_cast<apc::PadsMaster*>(ctrl)->get_status();
      push(master);
  });
  apc::PadsMaster::Get(4)->add_routine([&master](Controller::Control* ctrl){
      master.fade = static_cast<apc::PadsMaster*>(ctrl)->get_status();
      push(master);
  });

  apc::SequencerPads::Generate([&](uint8_t col, uint8_t row){
    apc::SequencerPads::Get(col, row)->add_routine([&sequencer](Controller::Control* ctrl){
      auto pad = static_cast<apc::SequencerPads*>(ctrl);
      uint8_t track = pad->getCol();
      uint8_t step = pad->getRow();
      uint8_t status = pad->get_status();
      uint8_t val = sequencer.steps[step];
      sequencer.steps[step] = (val & ~(1 << track)) | (status << track);
      // fprintf(stderr, "%02x %02x %02x, %d %d %d\n", sequencer.steps[0], sequencer.steps[1], sequencer.steps[2], track, step, status);
      push(sequencer);
    });
  });

  apc::Panic::Get()->add_routine([&](Controller::Control*){
    push(master);
    for (const auto& compo : compos)
      push(compo);
  });

  apc::StrobeParams::Get(0)->add_routine([&](Controller::Control* ctrl){
      master.strobe_speed = 32 - (static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 3);
      push(master);
      // fprintf(stderr, "speed\n");
  });
  apc::StrobeParams::Get(1)->add_routine([&](Controller::Control* ctrl){
      master.istimemod = static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 7;
      push(master);
      // fprintf(stderr, "timemod\n");
  });
  apc::StrobeParams::Get(2)->add_routine([&](Controller::Control* ctrl){
      master.pulse_width = static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 6;
      push(master);
      // fprintf(stderr, "pw\n");
  });
  apc::StrobeParams::Get(3)->add_routine([&](Controller::Control* ctrl){
    master.feedback = static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 1;
    push(master);
  });
  apc::StrobeEnable::Get()->add_routine([&](Controller::Control* ctrl){
      master.do_strobe = static_cast<apc::StrobeEnable*>(ctrl)->get_status();
      push(master);
      // fprintf(stderr, "strobe\n");
  });

  for (size_t osc = 0 ; osc < 3 ; ++osc)
  {
    apc::OscParams::Get(osc, 0)->add_routine([osc, &oscilators](Controller::Control* ctrl){
      oscilators[osc].kind = static_cast<apc::OscParams*>(ctrl)->get_value() >> 4;
      push(oscilators[osc]);
    });
    apc::OscParams::Get(osc, 1)->add_routine([osc, &oscilators](Controller::Control* ctrl){
      oscilators[osc].param1 = static_cast<apc::OscParams*>(ctrl)->get_value() >> 1;
      push(oscilators[osc]);
    });
    apc::OscParams::Get(osc, 2)->add_routine([osc, &oscilators](Controller::Control* ctrl){
      oscilators[osc].subdivide = static_cast<apc::OscParams*>(ctrl)->get_value() >> 5;
      push(oscilators[osc]);
    });
    apc::OscParams::Get(osc, 3)->add_routine([osc, &oscilators](Controller::Control* ctrl){
      oscilators[osc].source = static_cast<apc::OscParams*>(ctrl)->get_value() >> 6;
      push(oscilators[osc]);
    });
  }

  for (size_t bank = 0 ; bank < apc::TracksCount ; ++bank)
  {
    apc::PadsMatrix::Get(bank, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].map_on_index = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
      fprintf(stderr, "map_on_index %d\n", compos[bank].map_on_index);
      push(compos[bank]);
    });
    apc::PadsMatrix::Get(bank, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].effect1 = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
      fprintf(stderr, "effect1 %d\n", compos[bank].effect1);
      push(compos[bank]);
    });
    apc::PadsMatrix::Get(bank, 2)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].blend_mask = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
      fprintf(stderr, "blend_mask %d\n", compos[bank].blend_mask);
      push(compos[bank]);
    });
    apc::PadsMatrix::Get(bank, 3)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].stars = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
      fprintf(stderr, "stars %d\n", compos[bank].stars);
      push(compos[bank]);
    });
    apc::PadsMatrix::Get(bank, 4)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].strobe = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
      fprintf(stderr, "strobe %d\n", compos[bank].strobe);
      push(compos[bank]);
    });
    apc::PadsMatrix::Get(bank, 5)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].trigger = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
      fprintf(stderr, "trigger %d\n", compos[bank].trigger);
      push(compos[bank]);
    });

    apc::BottomEncoders::Get(bank, 0, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].palette = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 4;
      push(compos[bank]);
    });
    apc::BottomEncoders::Get(bank, 0, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].palette_width = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
      push(compos[bank]);
    });
    apc::BottomEncoders::Get(bank, 1, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].mod_intensity = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
      push(compos[bank]);
    });
    apc::BottomEncoders::Get(bank, 1, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].speed = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 6;
      push(compos[bank]);
    });

    apc::BottomEncoders::Get(bank, 2, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].index_offset = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
      push(compos[bank]);
    });
    apc::BottomEncoders::Get(bank, 2, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].param1 = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
      push(compos[bank]);
    });
    apc::BottomEncoders::Get(bank, 3, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].blend_overlay = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
      push(compos[bank]);
    });
    apc::BottomEncoders::Get(bank, 3, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].param_stars = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
      push(compos[bank]);
    });

    apc::Faders::Get(bank)->add_routine([bank, &compos](Controller::Control* ctrl){
      compos[bank].brightness = static_cast<apc::Faders*>(ctrl)->get_value() >> 1;
      push(compos[bank]);
    });
  } // for each bank

  apc::MainFader::Get()->add_routine([&](Controller::Control* ctrl){
      master.brightness = static_cast<apc::MainFader*>(ctrl)->get_value();
      push(master);
  });
  

  apc::TapTempo::Get()->add_routine([&](Controller::Control*){
    jack_time_t t = jack_get_time();
    timebase.length = t - timebase.last_hit;
    timebase.last_hit = t;
    master.bpm = timebase.bpm();
    push(master);
  });

  apc::IncTempo::Get()->add_routine([&](Controller::Control* ctrl){
    master.sync_correction = (master.sync_correction + 10) % 255;
    push(master);
  });
  apc::DecTempo::Get()->add_routine([&](Controller::Control* ctrl){
    master.sync_correction = (master.sync_correction - 10) % 255;
    push(master);
  });

  apc::SyncPot::Get()->add_routine([&](Controller::Control* ctrl){
    uint8_t val = static_cast<apc::SyncPot*>(ctrl)->get_value();
    if (val < 64)
      timebase.length *= 0.96;
    else
      timebase.length *= 1.04;
    master.bpm = timebase.bpm();
    push(master);
  });
  
  apc::TrackSelect::Generate([&](uint8_t track){
    fprintf(stderr, "Track : %d\n", track);
    apc::TrackSelect::Get(track)->add_routine([track, &master](Controller::Control* ctrl){
      master.active_compo = track;
      fprintf(stderr, "Track : %d\n", track);
      push(master);
    });
  });

  save_file = argv[1];
  
  {
    master = {
      120,  // bpm
      0,    // sync  
      127,  // brigthness
      0
    };
    sequencer = {0};
    uint8_t idx = 0;
    for (auto& compo : compos)
      compo = { idx++, 0 };

    idx = 0;
    for (auto& osc : oscilators)
      osc = { idx++, 0 };

    for (auto& pair : arduinos)
    {
      auto& [arduino, setup] = pair;
      arduino->push(setup);
    }
  }

  if (load())
    { perror("Error reading file"); return __LINE__; }
    

  while (1)
  {
    APC40.update_dirty_controls();
    usleep(10);
  }
  
  return 0;
}

void init_objects()
{
}