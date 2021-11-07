#include "../driver/state.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

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

  bool saveable;

  std::string name;
  size_t      addr_offset;
  type_e      type;
  value_u     val;

  std::function<void(std::vector<const control_t*>&)> on_update;
};

std::vector<control_t>                      controls_list;
std::unordered_map<std::string, control_t*> controls_by_name;
std::unordered_map<size_t, control_t*>      controls_by_addr;


std::vector<const control_t*> process_cmd(const char* cmdstr)
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

  std::vector<const control_t*> result;
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
    ctrl->on_update(result);
    result.emplace_back(ctrl);
  }

  return result;
}

const char* path_of_save;
const char* path_from_arduino;
const char* path_to_arduino;

volatile int is_running = 1;

void nullfunction(std::vector<const control_t*>&) {}

void save(std::vector<const control_t*>& dirty_controls)
{
  FILE* file = fopen(path_of_save, "w");
  if (!file)
  {
    perror("fopen save");
    exit(EXIT_FAILURE);
  }
  for (auto& ctrl : controls_list)
  {
    if (!ctrl.saveable)
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

void load(std::vector<const control_t*>& dirty_controls)
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
    auto tmp = process_cmd(buffer);
    for (auto& t : tmp)
      dirty_controls.emplace_back(t);
  }
  fclose(file);
}

void register_controls()
{
  // Generate controls
  size_t offset;

  // triggers
  offset = offsetof(state_t, triggers);
  controls_list.emplace_back(control_t{ false, "save", offset + offsetof(state_t::triggers_t, save), control_t::UINT7, {0}, save});
  controls_list.emplace_back(control_t{ false, "load", offset + offsetof(state_t::triggers_t, load), control_t::UINT7, {0}, load});
  
  // setup
  offset = offsetof(state_t, setup);
  controls_list.emplace_back(control_t{ true, "ribbons-count", offset + offsetof(state_t::setup_t, ribbons_count), control_t::UINT7, {0}, nullfunction});

  for (size_t i=0 ; i<MAX_RIBBONS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ true, "ribbons-length:" + std::to_string(i), offset + offsetof(state_t::setup_t, ribbons_length) + i, control_t::UINT7, {0}, nullfunction });

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

  }
  else // 0 != pid : Parent : read from stdin to stdout and arduino
  {
    char buffer[512];
    FILE* arduino = fopen(path_to_arduino, "w");
    if (!arduino)
    {
      perror("fopen");
      exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Starting listening commands\n");
    while (is_running && fgets(buffer, 512, stdin))
    {
      auto result = process_cmd(buffer);
      for (auto& ctrl : result)
      {
        char rawmsg[512];
        size_t msglen = 0;
        fprintf(stdout, "%s ", ctrl->name.c_str());
        rawmsg[0] = (ctrl->addr_offset & 0xFF00) >> 8;
        rawmsg[1] = ctrl->addr_offset & 0xFF;
        switch (ctrl->type)
        {
        case control_t::UINT7:
          fprintf(stdout, "%0u", ctrl->val.u);
          rawmsg[2] = 1;
          rawmsg[3] = ctrl->val.u;
          msglen = 4;
          break;
        case control_t::BOOL:
          fprintf(stdout, "%c", ctrl->val.b ? 'y' : 'n');
          rawmsg[2] = 1;
          rawmsg[3] = ctrl->val.b ? 0xFF : 0x00;
          msglen = 4;
          break;
        case control_t::FLOAT:
          fprintf(stdout, "%0.0f", ctrl->val.f);
          rawmsg[2] = sizeof(float);
          memcpy(rawmsg+3, &ctrl->val.f, sizeof(float));
          msglen = 3 + sizeof(float);
          break;
        }
        fprintf(stdout, "\n");
        fwrite(rawmsg, 1, msglen, arduino);
      }
      fflush(arduino);
    }
  }

  return 0;
}