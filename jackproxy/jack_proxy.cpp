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
  std::array<objects::Preset, apc::PadsColumnsCount> presets;
  std::array<objects::Ribbon, apc::PadsColumnsCount> ribbons;
  std::array<objects::Group, 3> groups;
} Global;

void update_controller_internals()
{
  apc::GroupSelectPads::Generate([&](uint8_t col, uint8_t row){
    apc::GroupSelectPads::Get(col, row)->set_status(Global.ribbons[col].group == row);
  });

  for (size_t bank = 0 ; bank < apc::TracksCount ; ++bank)
  {
    for (size_t pad = 0 ; pad < 5 ; ++pad)
      apc::PadsMatrix::Get(bank, pad)->set_status(!!(Global.presets[bank].pads_states & (1 << pad)));
    for (size_t pot = 0 ; pot < 8 ; ++pot)
      apc::BottomEncoders::Get(bank, pot % 4, pot / 4)->set_value(Global.presets[bank].encoders[pot]);
    for (size_t toggle = 0 ; toggle < 4 ; ++toggle)
      apc::EffectsSwitches::Get(bank, toggle)->set_status(Global.presets[bank].switches & (1 << toggle));
  }

  for (size_t i = 0 ; i < 8 ; ++i)
    apc::PadsBottomRow::Get(i)->set_status(false);
  
  for (size_t i = 0 ; i < 8 ; ++i)
  {
    apc::GlobalEncoders::Get(i % 4, i / 4)->set_value(Global.master.encoders[i]);
  }
  //apc::PadsBottomRow::Get(Global.groups[group].preset)->set_status(true);

  //apc::MainFader::Get()->set_value(Global.master.brightness);
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
      case objects::flags::Preset:
      {
        objects::Preset tmp = result.read<objects::Preset>();
        Global.presets[tmp.index] = tmp;
        break;
      }
      case objects::flags::Group:
      {
        objects::Group tmp = result.read<objects::Group>();
        Global.groups[tmp.index] = tmp;
        break;
      }
      case objects::flags::Ribbon:
      {
        objects::Ribbon tmp = result.read<objects::Ribbon>();
        Global.ribbons[tmp.index] = tmp;
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
  for (auto& preset : Global.presets)
    save(preset);
  for (auto& ribbon : Global.ribbons)
    save(ribbon);
  for (auto& group : Global.groups)
    save(group);

  fclose(file);
  return 0;
}

struct {
  jack_time_t length = 0;
  jack_time_t last_hit = 0;

  uint16_t bpm() const {
    return (60lu * 1000lu * 1000lu * 100lu) / (length+1);
  }
} timebase;

apc::APC40& APC40 = apc::APC40::Get();
std::array<std::pair<Arduino*, objects::Setup>, 1> arduinos;

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
  
  auto& setup = Global.setup;
  auto& master = Global.master;
  auto& presets = Global.presets;
  auto& ribbons = Global.ribbons;
  auto& groups = Global.groups;

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


  apc::Save::Get()->add_post_routine([](auto){
    if (int err = save())
      perror("Warning Save Failure");
    else
      fprintf(stderr, "Save OK : '%s'\n", save_file);
  });
  apc::Load::Get()->add_post_routine([](auto){
    if (int err = load())
      perror("Warning Load Failure");
    else
      fprintf(stderr, "Load OK : '%s'\n", save_file);
  });

  // apc::PadsMaster::Get(0)->add_routine([&master](Controller::Control* ctrl){
  //     master.reverse = static_cast<apc::PadsMaster*>(ctrl)->get_status();
  //     push(master);
  // });
  // apc::PadsMaster::Get(1)->add_routine([&master](Controller::Control* ctrl){
  //     master.use_ribbon = static_cast<apc::PadsMaster*>(ctrl)->get_status();
  //     push(master);
  // });

  // apc::PadsMaster::Get(3)->add_routine([&master](Controller::Control* ctrl){
  //     master.blur = static_cast<apc::PadsMaster*>(ctrl)->get_status();
  //     push(master);
  // });
  // apc::PadsMaster::Get(4)->add_routine([&master](Controller::Control* ctrl){
  //     master.fade = static_cast<apc::PadsMaster*>(ctrl)->get_status();
  //     push(master);
  // });

  apc::GroupSelectPads::Generate([&](uint8_t col, uint8_t row){
    apc::GroupSelectPads::Get(col, row)->add_post_routine([&ribbons](Controller::Control* ctrl){
      auto pad = static_cast<apc::GroupSelectPads*>(ctrl);
      uint8_t ribbon = pad->getCol();
      uint8_t group = pad->getRow();
      uint8_t status = pad->get_status();
      
      if (pad->get_status())
      {
        ribbons[ribbon].group = group;
      }
      else
      {
      }

      push(ribbons[ribbon]);
    });
  });

  apc::Panic::Get()->add_post_routine([&](Controller::Control*){
    push(setup);
    push(master);
    for (const auto& compo : presets)
      push(compo);
    for (const auto& compo : ribbons)
      push(compo);
    for (const auto& compo : groups)
      push(compo);
    update_controller_internals();
    dirty_controller = true;
  });

  for (size_t i = 0 ; i < 8 ; ++i)
  {
    apc::GlobalEncoders::Get(i % 4, i / 4)->add_post_routine([&master, i](Controller::Control* ctrl){
      auto pot = static_cast<apc::GlobalEncoders*>(ctrl);
      master.encoders[i] = pot->get_value();
      push(master);
    });
  }

  apc::PadsBottomRow::Generate([&](uint8_t i){
    apc::PadsBottomRow::Get(i)->add_rt_routine([&](Controller::Control* ctrl){
      auto pad = static_cast<apc::PadsBottomRow*>(ctrl);
      if (pad->get_status())
      {
        uint8_t group = ribbons[i].group;
        groups[group].preset = i;
        for (size_t p = 0 ; p < 8 ; ++p)
          if (i != p && groups[ribbons[p].group].preset != p)
            apc::PadsBottomRow::Get(p)->set_status(false);
        // for (size_t g = 0 ; g < 3 ; ++g)
          // apc::PadsBottomRow::Get(groups[g].preset)->set_status(true);
      }
    });
    apc::PadsBottomRow::Get(i)->add_post_routine([&](Controller::Control* ctrl){
      push(groups[ribbons[i].group]);
    });
  });

  for (size_t preset = 0 ; preset <  8 ; ++preset)
  {
    for (size_t i = 0 ; i < 8 ; i++)
      apc::BottomEncoders::Get(preset, i % 4, i / 4)->add_post_routine([preset, i, &presets, &ribbons](Controller::Control* ctrl){
        auto pot = static_cast<apc::BottomEncoders*>(ctrl);
        presets[preset].encoders[i] = pot->get_value();
        // Tracers is global to the group
        if (false && (i == 3 || i == 7))
        {
          for (size_t p = 0 ; p < 8 ; ++p)
            if (ribbons[p].group == ribbons[preset].group)
            {
              presets[p].encoders[i] = pot->get_value();
              push(presets[p]);
            }
        }
        else
          push(presets[preset]);
      });
    for (size_t i = 0 ; i < 4 ; ++i)
      apc::EffectsSwitches::Get(preset, i)->add_post_routine([preset, i, &presets](Controller::Control* ctrl){
        auto pad = static_cast<apc::EffectsSwitches*>(ctrl);
        uint8_t mask = 1 << i;
        uint8_t val = presets[preset].switches;
        uint8_t x = pad->get_status() << i;
        presets[preset].switches = x | (val & ~mask);
        push(presets[preset]);
      });
    apc::Faders::Get(preset)->add_post_routine([preset, &presets](Controller::Control* ctrl){
      presets[preset].brightness = static_cast<apc::Faders*>(ctrl)->get_value();
      push(presets[preset]);
    });
  }

  apc::PadsMatrix::Generate([&presets, &ribbons](uint8_t preset, uint8_t i){
    apc::PadsMatrix::Get(preset, i)->add_post_routine([&presets, &ribbons, preset, i](Controller::Control* ctrl){
      if (false && (i == 3 || i == 4))
      {
        for (size_t p = 0 ; p < 8 ; ++p)
          if (ribbons[p].group == ribbons[preset].group)
          {        
            auto pad = static_cast<apc::PadsMatrix*>(ctrl);
            uint8_t mask = 1 << i;
            uint8_t val = presets[p].pads_states;
            uint8_t x = pad->get_status() << i;
            presets[p].pads_states = x | (val & ~mask);
            push(presets[p]);
          }
      }
      else
      {
        auto pad = static_cast<apc::PadsMatrix*>(ctrl);
        uint8_t mask = 1 << i;
        uint8_t val = presets[preset].pads_states;
        uint8_t x = pad->get_status() << i;
        presets[preset].pads_states = x | (val & ~mask);
        push(presets[preset]);
      }
    });
  });

  // apc::StrobeParams::Get(0)->add_routine([&](Controller::Control* ctrl){
  //     master.strobe_speed = 32 - (static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 3);
  //     push(master);
  //     // fprintf(stderr, "speed\n");
  // });
  // apc::StrobeParams::Get(1)->add_routine([&](Controller::Control* ctrl){
  //     master.istimemod = static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 7;
  //     push(master);
  //     // fprintf(stderr, "timemod\n");
  // });
  // apc::StrobeParams::Get(2)->add_routine([&](Controller::Control* ctrl){
  //     master.pulse_width = static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 6;
  //     push(master);
  //     // fprintf(stderr, "pw\n");
  // });
  // apc::StrobeParams::Get(3)->add_routine([&](Controller::Control* ctrl){
  //   master.feedback = static_cast<apc::StrobeParams*>(ctrl)->get_value() >> 1;
  //   push(master);
  // });
  // apc::StrobeEnable::Get()->add_routine([&](Controller::Control* ctrl){
  //     master.do_strobe = static_cast<apc::StrobeEnable*>(ctrl)->get_status();
  //     push(master);
  //     // fprintf(stderr, "strobe\n");
  // });

  // for (size_t osc = 0 ; osc < 3 ; ++osc)
  // {
  //   apc::OscParams::Get(osc, 0)->add_routine([osc, &oscilators](Controller::Control* ctrl){
  //     oscilators[osc].kind = static_cast<apc::OscParams*>(ctrl)->get_value() >> 4;
  //     push(oscilators[osc]);
  //   });
  //   apc::OscParams::Get(osc, 1)->add_routine([osc, &oscilators](Controller::Control* ctrl){
  //     oscilators[osc].param1 = static_cast<apc::OscParams*>(ctrl)->get_value() >> 1;
  //     push(oscilators[osc]);
  //   });
  //   apc::OscParams::Get(osc, 2)->add_routine([osc, &oscilators](Controller::Control* ctrl){
  //     oscilators[osc].subdivide = static_cast<apc::OscParams*>(ctrl)->get_value() >> 5;
  //     push(oscilators[osc]);
  //   });
  //   apc::OscParams::Get(osc, 3)->add_routine([osc, &oscilators](Controller::Control* ctrl){
  //     oscilators[osc].source = static_cast<apc::OscParams*>(ctrl)->get_value() >> 6;
  //     push(oscilators[osc]);
  //   });
  // }

  // for (size_t bank = 0 ; bank < apc::TracksCount ; ++bank)
  // {
  //   apc::PadsMatrix::Get(bank, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].map_on_index = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
  //     fprintf(stderr, "map_on_index %d\n", compos[bank].map_on_index);
  //     push(compos[bank]);
  //   });
  //   apc::PadsMatrix::Get(bank, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].effect1 = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
  //     fprintf(stderr, "effect1 %d\n", compos[bank].effect1);
  //     push(compos[bank]);
  //   });
  //   apc::PadsMatrix::Get(bank, 2)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].blend_mask = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
  //     fprintf(stderr, "blend_mask %d\n", compos[bank].blend_mask);
  //     push(compos[bank]);
  //   });
  //   apc::PadsMatrix::Get(bank, 3)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].stars = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
  //     fprintf(stderr, "stars %d\n", compos[bank].stars);
  //     push(compos[bank]);
  //   });
  //   apc::PadsMatrix::Get(bank, 4)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].strobe = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
  //     fprintf(stderr, "strobe %d\n", compos[bank].strobe);
  //     push(compos[bank]);
  //   });
  //   apc::PadsMatrix::Get(bank, 5)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].trigger = static_cast<apc::PadsMatrix*>(ctrl)->get_status();
  //     fprintf(stderr, "trigger %d\n", compos[bank].trigger);
  //     push(compos[bank]);
  //   });

  //   apc::BottomEncoders::Get(bank, 0, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].palette = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 4;
  //     push(compos[bank]);
  //   });
  //   apc::BottomEncoders::Get(bank, 0, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].palette_width = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
  //     push(compos[bank]);
  //   });
  //   apc::BottomEncoders::Get(bank, 1, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].mod_intensity = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
  //     push(compos[bank]);
  //   });
  //   apc::BottomEncoders::Get(bank, 1, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].speed = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 6;
  //     push(compos[bank]);
  //   });

  //   apc::BottomEncoders::Get(bank, 2, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].index_offset = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
  //     push(compos[bank]);
  //   });
  //   apc::BottomEncoders::Get(bank, 2, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].param1 = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
  //     push(compos[bank]);
  //   });
  //   apc::BottomEncoders::Get(bank, 3, 0)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].blend_overlay = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
  //     push(compos[bank]);
  //   });
  //   apc::BottomEncoders::Get(bank, 3, 1)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].param_stars = static_cast<apc::BottomEncoders*>(ctrl)->get_value() >> 1;
  //     push(compos[bank]);
  //   });

  //   apc::Faders::Get(bank)->add_routine([bank, &compos](Controller::Control* ctrl){
  //     compos[bank].brightness = static_cast<apc::Faders*>(ctrl)->get_value() >> 1;
  //     push(compos[bank]);
  //   });
  // } // for each bank

  apc::MainFader::Get()->add_post_routine([&](Controller::Control* ctrl){
      master.brightness = static_cast<apc::MainFader*>(ctrl)->get_value();
      push(master);
  });
  
  apc::StrobeFader::Get()->add_post_routine([&](Controller::Control* ctrl){
      master.strobe_speed = static_cast<apc::StrobeFader*>(ctrl)->get_value();
      push(master);
  });

  apc::TapTempo::Get()->add_rt_routine([&](Controller::Control*){
    jack_time_t t = jack_get_time();
    timebase.length = t - timebase.last_hit;
    timebase.last_hit = t;
    master.bpm = timebase.bpm();
  });
  apc::TapTempo::Get()->add_post_routine([&](Controller::Control*){
    push(master);
  });

  apc::IncTempo::Get()->add_post_routine([&](Controller::Control* ctrl){
    //master.sync_correction = (master.sync_correction + 10) % 255;
    push(master);
  });
  apc::DecTempo::Get()->add_post_routine([&](Controller::Control* ctrl){
    //master.sync_correction = (master.sync_correction - 10) % 255;
    push(master);
  });

  apc::SyncPot::Get()->add_post_routine([&](Controller::Control* ctrl){
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
    apc::TrackSelect::Get(track)->add_post_routine([track, &master](Controller::Control* ctrl){
      // master.active_compo = track;
      fprintf(stderr, "Track : %d\n", track);
      push(master);
    });
  });

  save_file = argv[1];

  if (load())
    { perror("Error reading file"); return __LINE__; }
    
  {
    // setup = {
    //   4,
    //   {2, 2, 2, 2}
    // };
    // master = {
    //   120,  // bpm
    //   0,    // sync  
    //   127,  // brigthness
    // };
    
    uint8_t idx = 0;
    for (auto& preset : presets)
      preset.index = idx++;

    idx = 0;
    for (auto& ribbon : ribbons)
      ribbon.index = idx++;

    idx = 0;
    for (auto& group : groups)
      group.index = idx++;

    for (auto& pair : arduinos)
    {
      auto& [arduino, setup] = pair;
      arduino->push(setup);
    }
  }

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