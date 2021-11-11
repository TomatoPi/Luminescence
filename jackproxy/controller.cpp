#include "../driver/state.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

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
    PHYSICAL      = NON_SAVEABLE | NON_LOADABLE | VOLATILE,
    TRIGGER       = NON_SAVEABLE | VOLATILE,
  };
  
  uint8_t flags;

  std::string name;
  size_t      addr_offset;
  type_e      type;
  value_u     val;

  std::function<void(control_t*, dirty_list_t&, value_u)> on_update;
};

std::vector<control_t>                      controls_list;
std::unordered_map<std::string, control_t*> controls_by_name;
std::unordered_map<size_t, control_t*>      controls_by_addr;




dirty_list_t process_cmd(const char* cmdstr)
{
  char cmd[64], arg[64];
  if (sscanf(cmdstr, "%s %s", cmd, arg) != 2)
  {
    fprintf(stderr, "Invalid command : %s\n", cmdstr);
    return {};
  }

  auto [begin, end] = controls_by_name.equal_range(cmd);
  if (begin == end) // key not found
  {
    fprintf(stderr, "Unbound control : %s\n", cmd);
    return {};
  }

  dirty_list_t result;
  for (auto itr = begin; itr != end; ++itr)
  {
    auto& [_, ctrl] = *itr;
    int vi;
    float vf;
    control_t::value_u val;
    switch(ctrl->type)
    {
    case control_t::UINT7:
      if (1 != sscanf(arg, "%u", &vi))
      {
        fprintf(stderr, "Invalid value : %s\n", arg);
        continue;
      }
      val.u = vi;
      break;
    case control_t::BOOL:
      if (arg[0] != 'y' && arg[0] != 'n')
      {
        fprintf(stderr, "Invalid value : %s\n", arg);
        continue;
      }
      val.b = arg[0] == 'y';
      break;
    case control_t::FLOAT:
      if (1 != sscanf(arg, "%f", &vf))
      {
        fprintf(stderr, "Invalid value : %s\n", arg);
        continue;
      }
      val.f = vf;
      break;
    }
    ctrl->on_update(ctrl, result, val);
  }

  return result;
}

const char* path_of_save;
const char* path_from_arduino;
const char* path_to_arduino;

std::vector<std::string> presets_list;
size_t current_preset_index;

volatile int is_running = 1;

void default_callback(control_t* ctrl, dirty_list_t& dirty_controls, control_t::value_u val)
{
  ctrl->val = val;
  dirty_controls.emplace_back(ctrl, false);
}
void toggle_callback(control_t* ctrl, dirty_list_t& dirty_controls, control_t::value_u val)
{
  if (val.b)
    ctrl->val.b = !ctrl->val.b;
  dirty_controls.emplace_back(ctrl, false);
}

void load_saves_list()
{
  FILE* file = fopen(path_of_save, "r");
  if (!file)
  {
    perror("fopen save");
    exit(EXIT_FAILURE);
  }
  char buffer[512];
  presets_list.clear();
  while (fgets(buffer, 512, file))
  {
    size_t len = strlen(buffer);
    if (len == 0)
      continue;
    if (buffer[len-1] == '\n')
      buffer[len-1] = '\0';
    FILE* tmp = fopen(buffer, "r");
    if (!tmp)
    {
      perror("verify preset file");
      fprintf(stderr, "ERROR : Invalid save file : %s\n", buffer);
      return;
    }
    fclose(tmp);
    presets_list.emplace_back(buffer);
  }
  if (presets_list.size() == 0)
  {
    fprintf(stderr, "ERROR : No presets\n");
    return;
  }
  if (presets_list.size() <= current_preset_index)
  {
    fprintf(stderr, "WARNING : Fallback to preset 0\n");
    current_preset_index = 0;
  }
}

void save(control_t*, dirty_list_t& dirty_controls, control_t::value_u)
{
  if (presets_list.size() == 0)
  {
    fprintf(stderr, "ERROR : Failed to save\n");
    return;
  }
  FILE* file = fopen(presets_list[current_preset_index].c_str(), "w");
  if (!file)
  {
    perror("fopen save");
    exit(EXIT_FAILURE);
  }
  for (auto& ctrl : controls_list)
  {
    if (ctrl.flags & control_t::NON_SAVEABLE)
      continue;
    
    fprintf(file, "%s ", ctrl.name.c_str());
    switch (ctrl.type)
    {
    case control_t::UINT7:
      fprintf(file, "%0u", ctrl.val.u);
      break;
    case control_t::BOOL:
      fprintf(file, "%c", ctrl.val.b ? 'y' : 'n');
      break;
    case control_t::FLOAT:
      fprintf(file, "%0.0f", ctrl.val.f);
      break;
    }
    fprintf(file, "\n");
  }
  fclose(file);
  fprintf(stderr, "Successfully saved : %s\n", presets_list[current_preset_index].c_str());
}

void load(control_t*, dirty_list_t& dirty_controls, control_t::value_u)
{
  load_saves_list();
  if (presets_list.size() == 0)
  {
    fprintf(stderr, "ERROR : Failed to load\n");
    return;
  }
  FILE* file = fopen(presets_list[current_preset_index].c_str(), "r");
  if (!file)
  {
    perror("fopen save");
    exit(EXIT_FAILURE);
  }
  dirty_controls.clear();
  char buffer[512];
  while (fgets(buffer, 512, file))
  {
    char cmd[64], arg[64];
    if (sscanf(buffer, "%s %s", cmd, arg) != 2)
    {
      fprintf(stderr, "Invalid save key : %s\n", buffer);
      continue;
    }

    auto [begin, end] = controls_by_name.equal_range(cmd);
    if (begin == end) // key not found
    {
      fprintf(stderr, "Unbound save key : %s\n", cmd);
      continue;
    }

    for (auto itr = begin; itr != end; ++itr)
    {
      auto& [_, ctrl] = *itr;
      int vi;
      float vf;
      switch(ctrl->type)
      {
      case control_t::UINT7:
        if (1 != sscanf(arg, "%u", &vi))
        {
          fprintf(stderr, "Invalid value : %s\n", arg);
          continue;
        }
        ctrl->val.u = vi;
        break;
      case control_t::BOOL:
        if (arg[0] != 'y' && arg[0] != 'n')
        {
          fprintf(stderr, "Invalid value : %s\n", arg);
          continue;
        }
        ctrl->val.b = arg[0] == 'y';
        break;
      case control_t::FLOAT:
        if (1 != sscanf(arg, "%f", &vf))
        {
          fprintf(stderr, "Invalid value : %s\n", arg);
          continue;
        }
        ctrl->val.f = vf;
        break;
      }
      dirty_controls.emplace_back(ctrl, true);
    }
  }
  fclose(file);
  fprintf(stderr, "Successfully Loaded : %s\n", presets_list[current_preset_index].c_str());
}

void register_controls()
{
  // Generate controls
  size_t offset;

  // triggers
  offset = offsetof(state_t, triggers);
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "save", offset + offsetof(state_t::triggers_t, save), control_t::BOOL, {0}, save});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "load", offset + offsetof(state_t::triggers_t, load), control_t::BOOL, {0}, load});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "next_preset", offset + offsetof(state_t::triggers_t, next_preset), control_t::BOOL, {0}, 
  [](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){
    current_preset_index = (current_preset_index+1) % presets_list.size();
    load(ctrl, dirty_contorls, val);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "prev_preset", offset + offsetof(state_t::triggers_t, prev_preset), control_t::BOOL, {0}, 
  [](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){
    if (presets_list.size() != 0)
    {
      if (current_preset_index == 0)
        current_preset_index = presets_list.size();
      current_preset_index = current_preset_index-1;
    }
    load(ctrl, dirty_contorls, val);
  }});

  controls_list.emplace_back(control_t{ control_t::TRIGGER, "reset_bpm", offset + offsetof(state_t::triggers_t, reset_bpm), control_t::BOOL, {0}, 
  [](control_t*, dirty_list_t& dirty_contorls, control_t::value_u){
    auto& bpm = controls_by_name["bpm"];
    bpm->val.f = 1200.0f;
    dirty_contorls.emplace_back(bpm, false);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "correct_bpm", offset + offsetof(state_t::triggers_t, correct_bpm), control_t::BOOL, {0}, 
  [](control_t*, dirty_list_t& dirty_contorls, control_t::value_u val){
    auto& bpm = controls_by_name["bpm"];
    bpm->val.f *= val.b ? 1.01f : 0.99f;
    dirty_contorls.emplace_back(bpm, false);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "sync_left", offset + offsetof(state_t::triggers_t, sync_left), control_t::BOOL, {0}, 
  [](control_t*, dirty_list_t& dirty_contorls, control_t::value_u){
    auto& sync_correction = controls_by_name["sync_correction"];
    sync_correction->val.u -= 10;
    dirty_contorls.emplace_back(sync_correction, false);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "sync_right", offset + offsetof(state_t::triggers_t, sync_right), control_t::BOOL, {0}, 
  [](control_t*, dirty_list_t& dirty_contorls, control_t::value_u){
    auto& sync_correction = controls_by_name["sync_correction"];
    sync_correction->val.u += 10;
    dirty_contorls.emplace_back(sync_correction, false);
  }});
  for (size_t i=0 ; i<SOLOS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ 0, "solo:" + std::to_string(i), offset + offsetof(state_t::triggers_t, solo) + i, control_t::BOOL, {0}, 
    [i](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){
      if (!val.b)
        return;
      ctrl->val.b = !ctrl->val.b;
      auto& solo_enable = controls_by_name["solo_enable"];
      auto& solo_index = controls_by_name["solo_index"];
      solo_enable->val.b = ctrl->val.b;
      solo_index->val.u = i;
      dirty_contorls.emplace_back(solo_enable, false);
      dirty_contorls.emplace_back(solo_index, false);
      for (size_t j=0 ; j<SOLOS_COUNT ; ++j)
      {
        auto& ctrl2 = controls_by_name["solo:" + std::to_string(j)];
        ctrl2->val.b = i == j ? ctrl->val.b : false;
        dirty_contorls.emplace_back(ctrl2, false);
      }
    }});

  // palettes
  for (size_t p=0 ; p<PALETTES_COUNT ; ++p)
    for (size_t c=0 ; c<3 ; ++c)
    {
      offset  = offsetof(state_t, palettes) 
              + p * sizeof(state_t::palette_t)
              + offsetof(state_t::palette_t, params)
              + c * sizeof(state_t::palette_t::params_t);

      controls_list.emplace_back(control_t{ 0, "min_value:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, min_value), control_t::UINT7, {0}, default_callback});
      controls_list.emplace_back(control_t{ 0, "max_value:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, max_value), control_t::UINT7, {0}, default_callback});
      controls_list.emplace_back(control_t{ 0, "frequency_times_60:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, frequency_times_60), control_t::UINT7, {0}, default_callback});
      controls_list.emplace_back(control_t{ 0, "phase:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, phase), control_t::UINT7, {0}, default_callback});
    }
  
  // setup
  offset = offsetof(state_t, setup);
  controls_list.emplace_back(control_t{ 0, "ribbons_count", offset + offsetof(state_t::setup_t, ribbons_count), control_t::UINT7, {0}, default_callback});

  for (size_t i=0 ; i<MAX_RIBBONS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ 0, "ribbons_lengths:" + std::to_string(i), offset + offsetof(state_t::setup_t, ribbons_lengths) + i, control_t::UINT7, {0}, default_callback });
  for (size_t i=0 ; i<SOLOS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ 0, "soloribbons_location:" + std::to_string(i), offset + offsetof(state_t::setup_t, soloribbons_location) + i, control_t::UINT7, {0}, default_callback });

  // master
  offset = offsetof(state_t, master);
  controls_list.emplace_back(control_t{ control_t::VOLATILE, "bpm",         offset + offsetof(state_t::master_t, bpm), control_t::FLOAT, {0}, default_callback});
  controls_list.emplace_back(control_t{ 0, "sync_correction", offset + offsetof(state_t::master_t, sync_correction), control_t::UINT7, {0}, default_callback});

  controls_list.emplace_back(control_t{ control_t::PHYSICAL, "brightness",    offset + offsetof(state_t::master_t, brightness),   control_t::UINT7, {0}, default_callback});
  controls_list.emplace_back(control_t{ control_t::PHYSICAL, "strobe_speed",  offset + offsetof(state_t::master_t, strobe_speed), control_t::UINT7, {0}, default_callback});

  controls_list.emplace_back(control_t{ 0, "blur_enable",    offset + offsetof(state_t::master_t, blur_enable),   control_t::BOOL, {0}, toggle_callback});
  controls_list.emplace_back(control_t{ control_t::VOLATILE, "blur_qty",    offset + offsetof(state_t::master_t, blur_qty),   control_t::UINT7, {0}, default_callback});
  
  controls_list.emplace_back(control_t{ 0, "solo_enable",    offset + offsetof(state_t::master_t, solo_enable),   control_t::BOOL, {0}, default_callback});
  controls_list.emplace_back(control_t{ 0, "solo_index",    offset + offsetof(state_t::master_t, solo_index),   control_t::UINT7, {0}, default_callback});
  controls_list.emplace_back(control_t{ control_t::VOLATILE, "solo_weak_dim",    offset + offsetof(state_t::master_t, solo_weak_dim),   control_t::UINT7, {0}, default_callback});
  controls_list.emplace_back(control_t{ control_t::VOLATILE, "solo_strong_dim",    offset + offsetof(state_t::master_t, solo_strong_dim),   control_t::UINT7, {0}, default_callback});
  
  controls_list.emplace_back(control_t{ 0, "do_kill_lights",    offset + offsetof(state_t::master_t, do_kill_lights),   control_t::BOOL, {0}, toggle_callback});

  // presets
  for (size_t p=0 ; p<PRESETS_COUNT ; ++p)
  {
    offset = offsetof(state_t, presets) + p * sizeof(state_t::preset_t);

    controls_list.emplace_back(control_t{ control_t::VOLATILE, "palette:" + std::to_string(p), offset + offsetof(state_t::preset_t, palette), control_t::UINT7, {0}, default_callback});
    
    controls_list.emplace_back(control_t{ 0, "colormod_enable:" + std::to_string(p), offset + offsetof(state_t::preset_t, colormod_enable), control_t::BOOL, {0}, toggle_callback});
    controls_list.emplace_back(control_t{ control_t::VOLATILE, "colormod_osc:" + std::to_string(p), offset + offsetof(state_t::preset_t, colormod_osc), control_t::UINT7, {0}, default_callback});
    controls_list.emplace_back(control_t{ control_t::VOLATILE, "colormod_width:" + std::to_string(p), offset + offsetof(state_t::preset_t, colormod_width), control_t::UINT7, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "colormod_move:" + std::to_string(p), offset + offsetof(state_t::preset_t, colormod_move), control_t::BOOL, {0}, default_callback});

    controls_list.emplace_back(control_t{ 0, "maskmod_enable:" + std::to_string(p), offset + offsetof(state_t::preset_t, maskmod_enable), control_t::BOOL, {0}, toggle_callback});
    controls_list.emplace_back(control_t{ control_t::VOLATILE, "maskmod_osc:" + std::to_string(p), offset + offsetof(state_t::preset_t, maskmod_osc), control_t::UINT7, {0}, default_callback});
    controls_list.emplace_back(control_t{ control_t::VOLATILE, "maskmod_width:" + std::to_string(p), offset + offsetof(state_t::preset_t, maskmod_width), control_t::UINT7, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "maskmod_move:" + std::to_string(p), offset + offsetof(state_t::preset_t, maskmod_move), control_t::BOOL, {0}, default_callback});

    controls_list.emplace_back(control_t{ control_t::VOLATILE, "slicer_nslices:" + std::to_string(p), offset + offsetof(state_t::preset_t, slicer_nslices), control_t::UINT7, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "slicer_useuneven:" + std::to_string(p), offset + offsetof(state_t::preset_t, slicer_useuneven), control_t::BOOL, {0}, toggle_callback});
    controls_list.emplace_back(control_t{ control_t::VOLATILE, "slicer_nuneven:" + std::to_string(p), offset + offsetof(state_t::preset_t, slicer_nuneven), control_t::UINT7, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "slicer_useflip:" + std::to_string(p), offset + offsetof(state_t::preset_t, slicer_useflip), control_t::BOOL, {0}, default_callback});

    controls_list.emplace_back(control_t{ 0, "feedback_enable:" + std::to_string(p), offset + offsetof(state_t::preset_t, feedback_enable), control_t::BOOL, {0}, toggle_callback});
    controls_list.emplace_back(control_t{ control_t::VOLATILE, "feedback_qty:" + std::to_string(p), offset + offsetof(state_t::preset_t, feedback_qty), control_t::UINT7, {0}, default_callback});

    controls_list.emplace_back(control_t{ 0, "strobe_enable:" + std::to_string(p), offset + offsetof(state_t::preset_t, strobe_enable), control_t::BOOL, {0}, toggle_callback});

    controls_list.emplace_back(control_t{ control_t::VOLATILE, "speed_scale:" + std::to_string(p), offset + offsetof(state_t::preset_t, speed_scale), control_t::UINT7, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "slicer_mergeribbon:" + std::to_string(p), offset + offsetof(state_t::preset_t, slicer_mergeribbon), control_t::BOOL, {0}, default_callback});

    controls_list.emplace_back(control_t{ control_t::PHYSICAL, "brightness:" + std::to_string(p), offset + offsetof(state_t::preset_t, brightness), control_t::UINT7, {0}, default_callback});

    controls_list.emplace_back(control_t{ 0, "is_active_on_master:" + std::to_string(p), offset + offsetof(state_t::preset_t, is_active_on_master), control_t::BOOL, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "is_active_on_solo:" + std::to_string(p), offset + offsetof(state_t::preset_t, is_active_on_solo), control_t::BOOL, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "do_ignore_solo:" + std::to_string(p), offset + offsetof(state_t::preset_t, do_ignore_solo), control_t::BOOL, {0}, default_callback});

    controls_list.emplace_back(control_t{ 0, "do_litmax:" + std::to_string(p), offset + offsetof(state_t::preset_t, do_litmax), control_t::BOOL, {0}, default_callback});
  }

  // Generate tables
  for (auto& ctrl : controls_list)
  {
    controls_by_name.emplace(ctrl.name, &ctrl);
    controls_by_addr.emplace(ctrl.addr_offset, &ctrl);
  }
}

int main(int argc, char* const argv[])
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage : %s <save-file> <rawstream-in> <rawstream-out>", argv[0]);
    exit(EXIT_FAILURE);
  }

  path_of_save = argv[1];
  path_from_arduino = argv[2];
  path_to_arduino = argv[3];

  register_controls();

  // Read and write plain commands from stdios
  // Read and write bytes stream commands from given files
  // Modify internal state according to both inputs
  // Internal state's modifications are converted to commands on both outputs

  pid_t cpid = fork();
  if (-1 == cpid)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (0 == cpid) // Child : read from rawin (arduino)
  {
    FILE* arduino = fopen(path_from_arduino, "r");
    if (!arduino)
    {
      perror("fopen from arduino");
      exit(EXIT_FAILURE);
    }
    while (is_running)
    {
      char buffer;
      ssize_t nread = fread(&buffer, 1, 1, arduino);
      if (-1 == nread)
      {
        perror("read from arduino");
        exit(EXIT_FAILURE);
      }
      fputc(buffer, stderr);
      usleep(10);
    }
  }
  else // 0 != pid : Parent : read from stdin to stdout and arduino
  {
    char buffer[512];
    FILE* arduino = fopen(path_to_arduino, "w");
    if (!arduino)
    {
      perror("fopen to arduino");
      exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Starting listening commands\n");
    while (is_running && fgets(buffer, 512, stdin))
    {
      auto result = process_cmd(buffer);
      for (auto& [ctrl, force] : result)
      {
        if (force || !(ctrl->flags & control_t::VOLATILE))
        {
          fprintf(stdout, "%s ", ctrl->name.c_str());
          switch (ctrl->type)
          {
          case control_t::UINT7:
            fprintf(stdout, "%0u", ctrl->val.u);
            break;
          case control_t::BOOL:
            fprintf(stdout, "%c", ctrl->val.b ? 'y' : 'n');
            break;
          case control_t::FLOAT:
            fprintf(stdout, "%0.0f", ctrl->val.f);
            break;
          }
          fprintf(stdout, "\n");
        }

        char rawmsg[512];
        size_t msglen = 0;
        rawmsg[0] = (ctrl->addr_offset & 0xFF00) >> 8;
        rawmsg[1] = ctrl->addr_offset & 0xFF;
        switch (ctrl->type)
        {
        case control_t::UINT7:
          rawmsg[2] = 1;
          rawmsg[3] = ctrl->val.u;
          msglen = 4;
          break;
        case control_t::BOOL:
          rawmsg[2] = 1;
          rawmsg[3] = ctrl->val.b ? 0x7F : 0x00;
          msglen = 4;
          break;
        case control_t::FLOAT:
          rawmsg[2] = sizeof(float);
          memcpy(rawmsg+3, &ctrl->val.f, sizeof(float));
          msglen = 3 + sizeof(float);
          break;
        }
        fwrite(rawmsg, 1, msglen, arduino);
      }
      fflush(stdout);
      fflush(arduino);
      usleep(10);
    }
    kill(cpid, SIGTERM);
  }

  return 0;
}