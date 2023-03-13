#include "manager.hpp"

#include "state.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

std::string control_t::to_command_string() const
{
  char tmp[512];
  switch (type)
  {
  case control_t::UINT7:
    sprintf(tmp, "%s %0u", name.c_str(), val.u);
    break;
  case control_t::BOOL:
    sprintf(tmp, "%s %c", name.c_str(), val.b ? 'y' : 'n');
    break;
  case control_t::FLOAT:
    sprintf(tmp, "%s %0.2f", name.c_str(), val.f);
    break;
  }
  return std::string(tmp);
}
std::vector<uint8_t> control_t::to_raw_message() const
{
  char rawmsg[512];
  size_t msglen = 0;
  rawmsg[0] = 0xFF;
  rawmsg[1] = (addr_offset & 0xFF00) >> 8;
  rawmsg[2] = addr_offset & 0xFF;
  switch (type)
  {
  case control_t::UINT7:
    rawmsg[3] = 1;
    rawmsg[4] = val.u;
    msglen = 5;
    break;
  case control_t::BOOL:
    rawmsg[3] = 1;
    rawmsg[4] = val.b ? 0x7F : 0x00;
    msglen = 5;
    break;
  case control_t::FLOAT:
    rawmsg[3] = sizeof(float);
    memcpy(rawmsg+4, &val.f, sizeof(float));
    msglen = 4 + sizeof(float);
    break;
  }
  std::vector<uint8_t> msg;
  msg.reserve(msglen);
  for (size_t i=0 ; i<msglen ; ++i)
    msg.push_back(rawmsg[i]);
  return msg;
}

void default_callback(control_t* ctrl, dirty_list_t& dirty_controls, control_t::value_u val)
{
  ctrl->val = val;
  dirty_controls.insert_or_assign(ctrl, false);
}
void toggle_callback(control_t* ctrl, dirty_list_t& dirty_controls, control_t::value_u val)
{
  if (val.b)
    ctrl->val.b = !ctrl->val.b;
  dirty_controls.insert_or_assign(ctrl, false);
}

Manager::Manager(const char* save_path, const char* setup_path) :
  controls_list(), controls_by_name(), controls_by_addr(),
  path_of_save(save_path), path_of_setup(setup_path),
  presets_list(), current_preset_index(0)
{
  // Generate controls
  size_t offset;

  // triggers
  offset = offsetof(state_t, triggers);
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "save", offset + offsetof(state_t::triggers_t, save), control_t::BOOL, {0}, [this](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){ this->save(ctrl, dirty_contorls, val); }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "load", offset + offsetof(state_t::triggers_t, load), control_t::BOOL, {0}, [this](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){ this->load(ctrl, dirty_contorls, val); }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "next_preset", offset + offsetof(state_t::triggers_t, next_preset), control_t::BOOL, {0}, 
  [this](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){
    current_preset_index = (current_preset_index+1) % presets_list.size();
    load(ctrl, dirty_contorls, val);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "prev_preset", offset + offsetof(state_t::triggers_t, prev_preset), control_t::BOOL, {0}, 
  [this](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){
    if (presets_list.size() != 0)
    {
      if (current_preset_index == 0)
        current_preset_index = presets_list.size();
      current_preset_index = current_preset_index-1;
    }
    load(ctrl, dirty_contorls, val);
  }});

  controls_list.emplace_back(control_t{ control_t::TRIGGER, "reset_bpm", offset + offsetof(state_t::triggers_t, reset_bpm), control_t::BOOL, {0}, 
  [this](control_t*, dirty_list_t& dirty_contorls, control_t::value_u){
    auto& bpm = controls_by_name["bpm"];
    bpm->val.f = 1200.0f;
    dirty_contorls.insert_or_assign(bpm, false);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "correct_bpm", offset + offsetof(state_t::triggers_t, correct_bpm), control_t::BOOL, {0}, 
  [this](control_t*, dirty_list_t& dirty_contorls, control_t::value_u val){
    auto& bpm = controls_by_name["bpm"];
    bpm->val.f *= val.b ? 1.01f : 0.99f;
    dirty_contorls.insert_or_assign(bpm, false);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "sync_left", offset + offsetof(state_t::triggers_t, sync_left), control_t::BOOL, {0}, 
  [this](control_t*, dirty_list_t& dirty_contorls, control_t::value_u){
    auto& sync_correction = controls_by_name["sync_correction"];
    sync_correction->val.u -= 10;
    dirty_contorls.insert_or_assign(sync_correction, false);
  }});
  controls_list.emplace_back(control_t{ control_t::TRIGGER, "sync_right", offset + offsetof(state_t::triggers_t, sync_right), control_t::BOOL, {0}, 
  [this](control_t*, dirty_list_t& dirty_contorls, control_t::value_u){
    auto& sync_correction = controls_by_name["sync_correction"];
    sync_correction->val.u += 10;
    dirty_contorls.insert_or_assign(sync_correction, false);
  }});
  for (size_t i=0 ; i<SOLOS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ 0, "solo:" + std::to_string(i), offset + offsetof(state_t::triggers_t, solo) + i, control_t::BOOL, {0}, 
    [i, this](control_t* ctrl, dirty_list_t& dirty_contorls, control_t::value_u val){
      ctrl->val.b = val.b;
      auto& solo_enable = controls_by_name["solo_enable"];
      auto& solo_index = controls_by_name["solo_index"];
      solo_enable->val.b = ctrl->val.b;
      solo_index->val.u = i;
      dirty_contorls.insert_or_assign(solo_enable, false);
      dirty_contorls.insert_or_assign(solo_index, false);
      for (size_t j=0 ; j<SOLOS_COUNT ; ++j)
      {
        auto& ctrl2 = controls_by_name["solo:" + std::to_string(j)];
        ctrl2->val.b = i == j ? ctrl->val.b : false;
        dirty_contorls.insert_or_assign(ctrl2, false);
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

      controls_list.emplace_back(control_t{ control_t::SETUP, "min_value:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, min_value), control_t::UINT7, {0}, default_callback});
      controls_list.emplace_back(control_t{ control_t::SETUP, "max_value:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, max_value), control_t::UINT7, {0}, default_callback});
      controls_list.emplace_back(control_t{ control_t::SETUP, "frequency_times_60:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, frequency_times_60), control_t::UINT7, {0}, default_callback});
      controls_list.emplace_back(control_t{ control_t::SETUP, "phase:" + std::to_string(p) + ":" + std::to_string(c), offset + offsetof(state_t::palette_t::params_t, phase), control_t::UINT7, {0}, default_callback});
    }
  
  // setup
  offset = offsetof(state_t, setup);
  controls_list.emplace_back(control_t{ control_t::SETUP, "ribbons_count", offset + offsetof(state_t::setup_t, ribbons_count), control_t::UINT7, {0}, default_callback});

  for (size_t i=0 ; i<MAX_RIBBONS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ control_t::SETUP, "ribbons_lengths:" + std::to_string(i), offset + offsetof(state_t::setup_t, ribbons_lengths) + i, control_t::UINT7, {0}, default_callback });
  for (size_t i=0 ; i<SOLOS_COUNT ; ++i)
    controls_list.emplace_back(control_t{ control_t::SETUP, "soloribbons_location:" + std::to_string(i), offset + offsetof(state_t::setup_t, soloribbons_location) + i, control_t::UINT7, {0}, default_callback });

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
  
  controls_list.emplace_back(control_t{ control_t::VOLATILE, "do_kill_lights",    offset + offsetof(state_t::master_t, do_kill_lights),   control_t::BOOL, {0}, default_callback});

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

    controls_list.emplace_back(control_t{ control_t::VOLATILE, "do_litmax:" + std::to_string(p), offset + offsetof(state_t::preset_t, do_litmax), control_t::BOOL, {0}, default_callback});
  }

  // Generate tables
  for (auto& ctrl : controls_list)
  {
    controls_by_name.emplace(ctrl.name, &ctrl);
    controls_by_addr.emplace(ctrl.addr_offset, &ctrl);
  }
}

dirty_list_t Manager::process_command(const std::string& cmdstr)
{
  char cmd[64], arg[64];
  if (sscanf(cmdstr.c_str(), "%s %s", cmd, arg) != 2)
  {
    fprintf(stderr, "Invalid command : %s\n", cmdstr.c_str());
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

void Manager::load_saves_list()
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
void Manager::load_file(const char* path, dirty_list_t& dirty_controls)
{
FILE* file = fopen(path, "r");
  if (!file)
  {
    perror("fopen save");
    exit(EXIT_FAILURE);
  }
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
      dirty_controls.insert_or_assign(ctrl, true);
    }
  }
  fclose(file);
  fprintf(stderr, "Successfully Loaded : %s\n", presets_list[current_preset_index].c_str());
}
void Manager::load(control_t*, dirty_list_t& dirty_controls, control_t::value_u)
{
  load_saves_list();
  if (presets_list.size() == 0)
  {
    fprintf(stderr, "ERROR : Failed to load\n");
    return;
  }
  dirty_controls.clear();
  load_file(path_of_setup, dirty_controls);
  load_file(presets_list[current_preset_index].c_str(), dirty_controls);
}

void Manager::save(control_t*, dirty_list_t& dirty_controls, control_t::value_u)
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
    if (ctrl.flags & (control_t::NON_SAVEABLE | control_t::SETUP))
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
