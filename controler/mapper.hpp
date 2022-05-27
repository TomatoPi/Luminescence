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

struct Mapper {

  std::vector<binding_t>                                  bindings_list;
  std::unordered_multimap<int16_t, const binding_t*>      midi_to_command_map;
  std::unordered_multimap<std::string, const binding_t*>  command_to_midi_map;

  Mapper();
  
  std::vector<std::string> midimsg_to_command(const std::vector<uint8_t>& msg);
  std::vector<std::vector<std::uint8_t>> command_to_midimsg(const std::string& cmd);
};

namespace mycelium {

  template <typename event_t, typename result_t, typename map_t, typename traits>
  std::pair<std::vector<result_t>, typename traits::trace>
  map_event(const event_t& event, const map_t& map)
  {
    auto [event_key, event_args] = traits::unpack_event(event);
    if (!traits::is_valid_event(event_key))
      return std::make_pair(std::vector<result_t>{}, traits::invalid_event(event));

    auto [begin, end] = traits::match(event_key, map);
    if (begin == end)
      return std::make_pair(std::vector<result_t>{}, traits::unbound_event(event));
    
    std::vector<result_t> result;
    for (auto itr = begin; itr != end; ++itr)
      result.emplace_back(traits::remap_event(itr, event_args));

    return std::make_pair(result, traits::success(event));
  }

  struct optopoulpe_map_traits {

    using trace = std::string;

    static std::pair<std::string, std::string> unpack_event(const std::string& cmd) noexcept
    {
      char cmdkey[64], arg[64];
      if (sscanf(cmd.c_str(), "%s %s", cmdkey, arg) != 2)
        return {};
      else
        return std::make_pair(cmdkey, arg);
    }
    static bool is_valid_event(const std::string& key) noexcept {
      return key.size() != 0;
    }
    static trace invalid_event(const std::string& cmd) noexcept {
      return "Invalid Event : " + cmd + "\n";
    }

    static constexpr auto match = [](const std::string& key, const std::unordered_multimap<std::string, const binding_t*>& map)
      -> decltype(map.equal_range(key)) { return map.equal_range(key); };

    static trace unbound_event(const std::string& cmd) noexcept {
      return "Unbound Event : " + cmd + "\n";
    }

    static std::vector<uint8_t> remap_event(const std::unordered_multimap<std::string, const binding_t*>::const_iterator& itr, const std::string& args)
    {
      auto& [_, binding] = *itr;
      uint8_t raw[3];
      binding->str_to_midival(binding, args.c_str(), raw);
      return {raw[0], raw[1], raw[2]};
    }

    static trace success(const std::string& cmd) noexcept {
      return "Success";
    }
  };
}