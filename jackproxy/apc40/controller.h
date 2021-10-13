#pragma once

#include "../midihash.h"

#include <cstdint>
#include <queue>
#include <array>
#include <unordered_set>
#include <optional>
#include <vector>
#include <mutex>
#include <memory>

#include <stdio.h>

class Controller
{
public:

  class Control
  {
  public :
    using RealtimeRoutine = std::function<void(Control* ctrl)>;
    using PostRoutine = std::function<void(Control* ctrl)>;

  protected:
    Controller* controller = nullptr;
    std::vector<RealtimeRoutine> realtime_routines;
    std::vector<PostRoutine> post_routines;

    
  public:
    Control(Controller* ctrl = nullptr) : controller(ctrl) {}
    virtual ~Control() = default;

    virtual void push_refresh(bool force) = 0;

    void exec_rt_routines()
    {
      for (auto& routine : realtime_routines)
        routine(this);
    }

    void add_rt_routine(const RealtimeRoutine& rt)
    {
      realtime_routines.emplace_back(rt);
    }


    void exec_post_routines()
    {
      for (auto& routine : post_routines)
        routine(this);
    }

    void add_post_routine(const PostRoutine& rt)
    {
      post_routines.emplace_back(rt);
    }

    void set_dirty()
    {
      controller->dirty_controls.emplace(this);
    }
  };

private:

  using Controls = std::vector<std::unique_ptr<Control>>;
  using MidiStack = std::vector<MidiMsg>;

  using MappingsTable = std::unordered_multimap<
    MidiMsg, 
    std::pair<Control*, MidiMsgHandler>, 
    MidiMsgHash, MidiMsgEquals>;

  Controls controls;
  MappingsTable midi_map;
  MidiStack rt_queue;

  std::unordered_set<Control*> dirty_controls;
  std::mutex lock;

public:

  Controller() = default;
  ~Controller() = default;

  template <typename T, typename ...Args>
  Control* addControl(Args&& ...args)
  {
    return controls.emplace_back(std::make_unique<T>(this, std::forward<Args>(args)...)).get();
  }
  void register_mapping(Control* ctrl, const MidiMsg& signature, const MidiMsgHandler& callback)
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

    for (size_t i = 0 ; i < 4 ; ++i)
    {
      fprintf(stderr, "0x%02x ", ((uint8_t*)&event)[i]);
    }
    fprintf(stderr, " %08lx\n", MidiMsgHash()(event));

    rt_queue.clear();
    auto [begin, end] = midi_map.equal_range(event);
    std::scoped_lock<std::mutex> _(lock);
    for (auto itr = begin ; itr != end ; ++itr)
    {
      auto& [key, pair] = *itr;
      auto& [ctrl, callback] = pair;
      callback(event);
      ctrl->exec_rt_routines();
      ctrl->push_refresh(false);
      ctrl->set_dirty();
    }
    return rt_queue;
  }

  MidiStack& refresh_all_controllers()
  {
    rt_queue.clear();
    for (const auto& control : controls)
      control->push_refresh(true);
    return rt_queue;
  }

  void update_dirty_controls()
  {
    std::scoped_lock<std::mutex> _(lock);
    for (auto& ctrl : dirty_controls)
      ctrl->exec_post_routines();
    dirty_controls.clear();
  }

  void dump()
  {
    fprintf(stderr, "MidiMap\n");
    for (auto& itr : midi_map)
    {
      auto& [key, pair] = itr;
      auto& [ctrl, _] = pair;
      fprintf(stderr, "%02x %02x %02x %02x : %p\n", key.s, key.c, key.d1, key.d2, ctrl);
    }
  }
};
