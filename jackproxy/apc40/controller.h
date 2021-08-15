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

  class Control
  {
  protected:
    Controller* controller = nullptr;

  public:
    Control(Controller* ctrl = nullptr) : controller(ctrl) {}
    virtual ~Control() = default;

    virtual void send_refresh() = 0;
  };

private:

  using Controls = std::vector<std::unique_ptr<Control>>;
  using MidiStack = std::vector<MidiMsg>;

  Controls controls;
  MidiHashMap midi_map;
  MidiStack rt_queue;

public:

  Controller() = default;
  ~Controller() = default;

  template <typename T, typename ...Args>
  Control* addControl(Args&& ...args)
  {
    return controls.emplace_back(std::make_unique<T>(this, std::forward<Args>(args)...)).get();
  }
  void register_mapping(const MidiMsg& signature, const MidiCallback& callback)
  {
    midi_map.emplace(signature, callback);
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
    if (
      auto itr = midi_map.find(event);
      itr != midi_map.end())
    {
      auto& [_, callback] = *itr;
      callback(event);
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
};
