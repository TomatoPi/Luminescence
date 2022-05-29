#pragma once

#include <string>
#include <vector>
#include <optional>
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

  std::unordered_multimap<int16_t, const binding_t*>      midi_to_command_map;
  std::unordered_multimap<std::string, const binding_t*>  command_to_midi_map;

  static const std::vector<binding_t>& APC40_mappings();

  Mapper(const std::vector<binding_t>& bindings_list);
  
  std::vector<std::string> midimsg_to_command(const std::vector<uint8_t>& msg);
  std::vector<std::vector<std::uint8_t>> command_to_midimsg(const std::string& cmd);
};

namespace mycelium {

  template <class event_t, class table_t, class traits_t>
  auto remap(const event_t& event, const table_t& table, const traits_t& traits)
  {
    return traits.unpack(event) | traits.match(table) | traits.remap();
  }

  template <
    typename event_t,
    typename map_t,
    typename traits
    >
  auto map_event(const event_t& event, const map_t& map)
  {
    auto unpacked_event = traits::unpack(event);
    if (!traits::is_valid(unpacked_event))
      return traits::invalid(event);

    auto matches = traits::match(unpacked_event, map);
    if (traits::is_empty(matches))
      return traits::unbound(unpacked_event);
    
    return traits::remap(matches, unpacked_event);
  }

  struct optopoulpe_map_traits {

    using event_t = std::string;
    using map_t = std::unordered_multimap<event_t, const binding_t*>;

    using key_args_t = std::pair<std::string, std::string>;
    using unpacked_event_t = std::optional<key_args_t>;
    using matches_t = std::pair<map_t::const_iterator, map_t::const_iterator>;

    using midi_t = std::vector<uint8_t>;
    using midiv_t = std::vector<midi_t>;
    using result_t = std::pair<midiv_t, std::string>;

    static unpacked_event_t unpack(const std::string& cmd) noexcept
    {
      char cmdkey[64], arg[64];
      if (sscanf(cmd.c_str(), "%s %s", cmdkey, arg) != 2)
        return std::nullopt;
      else
        return std::make_optional(std::make_pair(cmdkey, arg));
    }
    static bool is_valid(const unpacked_event_t& event) noexcept {
      return event.has_value();
    }
    static result_t invalid(const event_t& event) noexcept {
      return {{}, "Invalid Event : " + event + "\n"};
    }

    static constexpr auto match = [](const unpacked_event_t& event, const auto& map)
      -> decltype(map.equal_range(event.value().first)) { return map.equal_range(event.value().first); };

    static bool is_empty(const matches_t& range) noexcept {
      return range.first == range.second;
    }

    static result_t unbound(const unpacked_event_t& event) noexcept {
      return {{}, "Unbound Event : " + event.value().first + "\n"};
    }

    static result_t remap(const matches_t& range, const unpacked_event_t& event)
    {
      auto remap_item = [](const auto& itr, const auto& args) {
        auto& [_, binding] = *itr;
        uint8_t raw[3];
        binding->str_to_midival(binding, args.c_str(), raw);
        return midi_t{raw[0], raw[1], raw[2]};
      };
      
      midiv_t result;
      for (auto itr = range.first; itr != range.second; ++itr)
        result.emplace_back(remap_item(itr, event.value().second));
      return {result, "Success"};
    }

  };
}