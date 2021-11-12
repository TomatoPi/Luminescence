#include "mapper.hpp"

std::string uint8_to_str(const binding_t* bnd, uint8_t val)
{
  return std::to_string(val);
}
void str_to_uint8(const binding_t* bnd, const char* str, uint8_t val[3])
{
  int res;
  sscanf(str, "%d", &res);
  val[0] = bnd->midikey[0];
  val[1] = bnd->midikey[1];
  val[2] = res;
}

std::string bool_to_str(const binding_t* bnd, uint8_t val)
{
  return 0x40 <= val ? "y" : "n";
}
void str_to_bool(const binding_t* bnd, const char* str, uint8_t val[3])
{
  val[0] = bnd->midikey[0];
  val[1] = bnd->midikey[1];
  val[2] = str[0] == 'y' ? 0x7F : 0x00;
}

std::string pad_to_str(const binding_t* bnd, uint8_t val)
{
  return 0x90 == (bnd->midikey[0] & 0xF0) ? "y" : "n";
}
void str_to_pad(const binding_t* bnd, const char* str, uint8_t val[3])
{
  val[0] = str[0] == 'y' ? 0x90 : 0x80;
  val[0] |= bnd->midikey[0] & 0x0F;
  val[1] = bnd->midikey[1];
  val[2] = 0x7F;
}

Mapper::Mapper()
{
  // Generate bindings
  bindings_list = {
    { {0x90, 0x5b}, "load", bool_to_str, str_to_bool},
    { {0x90, 0x5d}, "save", bool_to_str, str_to_bool},
    { {0x90, 0x61}, "prev_preset", bool_to_str, str_to_bool},
    { {0x90, 0x60}, "next_preset", bool_to_str, str_to_bool},

    { {0x90, 0x63}, "reset_bpm", bool_to_str, str_to_bool},
    { {0xb0, 0x2f}, "correct_bpm", bool_to_str, str_to_bool},
    { {0x90, 0x65}, "sync_left", bool_to_str, str_to_bool},
    { {0x90, 0x64}, "sync_right", bool_to_str, str_to_bool},

    { {0xb0, 0x0e}, "brightness", uint8_to_str, str_to_uint8},
    { {0xb0, 0x0f}, "strobe_speed", uint8_to_str, str_to_uint8},

    { {0xb8, 0x13}, "blur_qty", uint8_to_str, str_to_uint8},

    { {0xb8, 0x10}, "solo_weak_dim", uint8_to_str, str_to_uint8},
    { {0xb8, 0x14}, "solo_strong_dim", uint8_to_str, str_to_uint8},
  };

  for (uint8_t o=0 ; o <= 0x10 ; o += 0x10)
  {
    bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + o), 0x55}, "blur_enable", pad_to_str, str_to_pad});
    bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + o), 0x51}, "do_kill_lights", pad_to_str, str_to_pad});

    for (uint8_t i=0 ; i<4 ; ++i)
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + o), (uint8_t)(0x57 + i)}, "solo:" + std::to_string(i), pad_to_str, str_to_pad});
  }

  for (uint8_t p=0 ; p < 8 ; ++p)
  {
    bindings_list.emplace_back(binding_t{{0xb0, (uint8_t)(0x30 + p)}, "palette:" + std::to_string(p), uint8_to_str, str_to_uint8});

    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x10}, "colormod_osc:"   + std::to_string(p), uint8_to_str, str_to_uint8});
    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x14}, "colormod_width:" + std::to_string(p), uint8_to_str, str_to_uint8});

    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x11}, "maskmod_osc:"    + std::to_string(p), uint8_to_str, str_to_uint8});
    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x15}, "maskmod_width:"  + std::to_string(p), uint8_to_str, str_to_uint8});

    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x12}, "slicer_nslices:" + std::to_string(p), uint8_to_str, str_to_uint8});
    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x16}, "slicer_nuneven:" + std::to_string(p), uint8_to_str, str_to_uint8});

    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x13}, "feedback_qty:"   + std::to_string(p), uint8_to_str, str_to_uint8});

    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x17}, "speed_scale:" + std::to_string(p), uint8_to_str, str_to_uint8});

    bindings_list.emplace_back(binding_t{{(uint8_t)(0xb0 + p), 0x07}, "brightness:" + std::to_string(p), uint8_to_str, str_to_uint8});

    for (uint8_t o=0 ; o <= 0x10 ; o += 0x10)
    {
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x35}, "colormod_enable:"  + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x3a}, "colormod_move:"    + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x36}, "maskmod_enable:"   + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x3b}, "maskmod_move:"     + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x37}, "slicer_useuneven:" + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x3c}, "slicer_useflip:"   + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x38}, "feedback_enable:"  + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x39}, "strobe_enable:"    + std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x3d}, "slicer_mergeribbon:"+ std::to_string(p), pad_to_str, str_to_pad});

      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x32}, "is_active_on_master:"+ std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x31}, "is_active_on_solo:"+ std::to_string(p), pad_to_str, str_to_pad});
      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x30}, "do_ignore_solo:"+ std::to_string(p), pad_to_str, str_to_pad});

      bindings_list.emplace_back(binding_t{{(uint8_t)(0x80 + p + o), 0x34}, "do_litmax:"+ std::to_string(p), pad_to_str, str_to_pad});
    }
  }

  // Generate tables
  for (const auto& binding : bindings_list)
  {
    midi_to_command_map.emplace(binding.key(), &binding);
    command_to_midi_map.emplace(binding.command, &binding);
  }
}

std::vector<std::string> Mapper::midimsg_to_command(const std::vector<uint8_t>& msg)
{
  // convert raw midi in 'msg' to command str
  if (msg.size() != 3)
  {
    fprintf(stderr, "Unsupported midimsg : %lu\n", msg.size());
    return {};
  }

  const uint16_t key = ((uint16_t)msg[0]) << 8 | msg[1];
  // search for key in mappings
  auto [begin, end] = midi_to_command_map.equal_range(key);
  if (begin == end) // key not found
  {
    fprintf(stderr, "Unbounded midi msg %04x\n", key);
    return {};
  }

  const uint8_t value = msg[2];
  std::vector<std::string> result;
  for (auto itr = begin; itr != end; ++itr)
  {
    auto& [_, binding] = *itr;
    char tmp[512];
    sprintf(tmp, "%s %s", binding->command.c_str(), binding->midival_to_str(binding, value).c_str());
    result.emplace_back(tmp);
  }

  return result;
}
std::vector<std::vector<std::uint8_t>> Mapper::command_to_midimsg(const std::string& cmd)
{
  char cmdkey[64], arg[64];
  if (sscanf(cmd.c_str(), "%s %s", cmdkey, arg) != 2)
  {
    fprintf(stderr, "Invalid command : %s\n", cmd.c_str());
    return {};
  }

  auto [begin, end] = command_to_midi_map.equal_range(cmdkey);
  if (begin == end) // key not found
  {
    fprintf(stderr, "Unbound command : %s\n", cmdkey);
    return {};
  }

  std::vector<std::vector<uint8_t>> result;
  for (auto itr = begin; itr != end; ++itr)
  {
    auto& [_, binding] = *itr;
    uint8_t raw[3];
    binding->str_to_midival(binding, arg, raw);

    std::vector<uint8_t> msg = {raw[0], raw[1], raw[2]};
    result.emplace_back(msg);
  }
  
  return result;
}