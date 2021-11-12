#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

struct binding_t
{
  uint8_t         midikey[2];
  std::string     command;
  std::function<std::string(const binding_t*, uint8_t)>           midival_to_str;
  std::function<void(const binding_t*, const char*, uint8_t[3])>  str_to_midival;

  uint16_t key() const { return ((uint16_t)midikey[0]) << 8 | midikey[1]; }
};

class Mapper {

  std::vector<binding_t>                                  bindings_list;
  std::unordered_multimap<int16_t, const binding_t*>      midi_to_command_map;
  std::unordered_multimap<std::string, const binding_t*>  command_to_midi_map;

public :

  Mapper();
  
  std::vector<std::string> midimsg_to_command(const std::vector<uint8_t>& msg);
  std::vector<std::vector<std::uint8_t>> command_to_midimsg(const std::string& cmd);
};