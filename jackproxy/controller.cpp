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
    VOLATILE      = 0x02,
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

void save(control_t*, dirty_list_t& dirty_controls, control_t::value_u)
{
  FILE* file = fopen(path_of_save, "w");
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
}

void load(control_t*, dirty_list_t& dirty_controls, control_t::value_u)
{
  FILE* file = fopen(path_of_save, "r");
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
}

void register_controls()
{
  // Generate controls
  size_t offset;

  // triggers
  offset = offsetof(state_t, triggers);
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "save", offset + offsetof(state_t::triggers_t, save), control_t::BOOL, {0}, save});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "load", offset + offsetof(state_t::triggers_t, load), control_t::BOOL, {0}, load});

  controls_list.emplace_back(control_t{ control_t::TRIGGER, "reset_bpm", offset + offsetof(state_t::triggers_t, reset_bpm), control_t::BOOL, {0}, 
  [](control_t*, dirty_list_t& dirty_contorls, control_t::value_u){
    auto& bpm = controls_by_name["bpm"];
    bpm->val.f = 6000.0f;
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
  
  // setup
  offset = offsetof(state_t, setup);
  controls_list.emplace_back(control_t{ 0, "ribbons_count", offset + offsetof(state_t::setup_t, ribbons_count), control_t::UINT7, {0}, default_callback});

  for (size_t i=0 ; i<MAX_RIBBONS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ 0, "ribbons_lengths:" + std::to_string(i), offset + offsetof(state_t::setup_t, ribbons_lengths) + i, control_t::UINT7, {0}, default_callback });

  // master
  offset = offsetof(state_t, master);
  controls_list.emplace_back(control_t{ control_t::VOLATILE, "bpm",         offset + offsetof(state_t::master_t, bpm), control_t::FLOAT, {0}, default_callback});
  controls_list.emplace_back(control_t{ 0, "sync_correction", offset + offsetof(state_t::master_t, sync_correction), control_t::UINT7, {0}, default_callback});

  controls_list.emplace_back(control_t{ control_t::VOLATILE, "brightness",    offset + offsetof(state_t::master_t, brightness),   control_t::UINT7, {0}, default_callback});
  controls_list.emplace_back(control_t{ 0, "strobe_speed",  offset + offsetof(state_t::master_t, strobe_speed), control_t::UINT7, {0}, default_callback});

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
    controls_list.emplace_back(control_t{ 0, "slicer_useflip:" + std::to_string(p), offset + offsetof(state_t::preset_t, slicer_useflip), control_t::BOOL, {0}, default_callback});
    controls_list.emplace_back(control_t{ 0, "slicer_useuneven:" + std::to_string(p), offset + offsetof(state_t::preset_t, slicer_useuneven), control_t::BOOL, {0}, toggle_callback});

    controls_list.emplace_back(control_t{ 0, "feedback_enable:" + std::to_string(p), offset + offsetof(state_t::preset_t, feedback_enable), control_t::BOOL, {0}, toggle_callback});
    controls_list.emplace_back(control_t{ control_t::VOLATILE, "feedback_qty:" + std::to_string(p), offset + offsetof(state_t::preset_t, feedback_qty), control_t::UINT7, {0}, default_callback});

    controls_list.emplace_back(control_t{ 0, "strobe_enable:" + std::to_string(p), offset + offsetof(state_t::preset_t, strobe_enable), control_t::BOOL, {0}, toggle_callback});

    controls_list.emplace_back(control_t{ control_t::VOLATILE, "speed_scale:" + std::to_string(p), offset + offsetof(state_t::preset_t, speed_scale), control_t::UINT7, {0}, default_callback});

    controls_list.emplace_back(control_t{ control_t::VOLATILE, "brightness:" + std::to_string(p), offset + offsetof(state_t::preset_t, brightness), control_t::UINT7, {0}, default_callback});
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