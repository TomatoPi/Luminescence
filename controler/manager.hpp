#pragma once

#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <unordered_map>

struct control_t;
using dirty_list_t = std::vector<std::pair<const control_t*, bool>>; // (ctrl, force update)

struct control_t
{
  enum type_e {
    UINT7,
    BOOL,
    FLOAT,
  };
  union value_u {
    uint8_t u;
    bool    b;
    float   f;
  };

  enum flags_e {
    NONE          = 0,
    NON_SAVEABLE  = 0x01,
    NON_LOADABLE  = 0x02,
    VOLATILE      = 0x04,
    SETUP         = 0x80,
    PHYSICAL      = NON_SAVEABLE | NON_LOADABLE | VOLATILE,
    TRIGGER       = NON_SAVEABLE | VOLATILE,
  };
  
  uint8_t flags;

  std::string name;
  size_t      addr_offset;
  type_e      type;
  value_u     val;

  std::function<void(control_t*, dirty_list_t&, value_u)> on_update;

  std::string to_command_string() const;
  std::vector<uint8_t> to_raw_message() const;
};

class Manager {
  std::vector<control_t>                      controls_list;
  std::unordered_map<std::string, control_t*> controls_by_name;
  std::unordered_map<size_t, control_t*>      controls_by_addr;

  const char* path_of_save;
  const char* path_of_setup;

  std::vector<std::string> presets_list;
  size_t current_preset_index;

public:

  Manager(const char* save_path, const char* setup_path);

  dirty_list_t process_command(const std::string& cmd);

private:

  void load_saves_list();
  void load_file(const char* path, dirty_list_t& dirty_controls);
  void load(control_t*, dirty_list_t& dirty_controls, control_t::value_u);

  void save(control_t*, dirty_list_t& dirty_controls, control_t::value_u);

};