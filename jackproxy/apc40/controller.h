#pragma once

#include "../midihash.h"

#include <cstdint>
#include <queue>
#include <array>
#include <unordered_set>
#include <vector>
#include <memory>

#include <stdio.h>

class Controller
{
public:

  class Control;
  using Routine = std::function<void(Control*)>;

  class Control
  {
  protected:
    Controller* controller = nullptr;
    std::vector<Routine> routines;

  public:
    Control(Controller* ctrl = nullptr) : controller(ctrl) {}
    virtual ~Control() = default;

    virtual void send_refresh() = 0;

    void exec_routines()
    {
      for (auto& routine : routines)
        routine(this);
    }

    void add_routine(const Routine& rt)
    {
      routines.emplace_back(rt);
      fprintf(stderr, "New routine for %p\n", this);
    }
  };

private:

  using Controls = std::vector<std::unique_ptr<Control>>;
  using MidiStack = std::vector<MidiMsg>;

  using MappingsTable = std::unordered_multimap<MidiMsg, std::pair<Control*, MidiCallback>, MidiMsgHash, MidiMsgEquals>;

  Controls controls;
  MappingsTable midi_map;
  MidiStack rt_queue;

  std::unordered_set<Control*> dirty_controls;

public:

  Controller() = default;
  ~Controller() = default;

  template <typename T, typename ...Args>
  Control* addControl(Args&& ...args)
  {
    return controls.emplace_back(std::make_unique<T>(this, std::forward<Args>(args)...)).get();
  }
  void register_mapping(Control* ctrl, const MidiMsg& signature, const MidiCallback& callback)
  {
    midi_map.emplace(signature, std::make_pair(ctrl, callback));
  }

  template <typename ...Args>
  void push_event(Args&& ...args)
  {
    rt_queue.emplace_back(MidiMsg(std::forward<Args>(args)...));
  }

  template <typename ...Args>
  void push_event(std::initializer_list<std::initializer_list<Args...>>&& events)
  {
    rt_queue.reserve(rt_queue.size() + events.size());
    for (const auto& event : events)
      push_event(event);
  }

  template <typename ...Args>
  MidiStack& handle_midi_event(Args&& ...args)
  {
    MidiMsg event(std::forward<Args>(args)...);
    rt_queue.clear();
    auto [begin, end] = midi_map.equal_range(event);
    for (auto itr = begin ; itr != end ; ++itr)
    {
      auto& [_, pair] = *itr;
      auto& [ctrl, callback] = pair;
      callback(event);
      dirty_controls.emplace(ctrl);
    }
    return rt_queue;
  }

  MidiStack& refresh_all_controllers()
  {
    rt_queue.clear();
    for (const auto& control : controls)
      control->send_refresh();
    return rt_queue;
  }

  void update_dirty_controls()
  {
    for (auto& ctrl : dirty_controls)
      ctrl->exec_routines();
    dirty_controls.clear();
  }
};
