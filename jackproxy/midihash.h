#pragma once

#include <vector>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <unordered_map>

struct MidiMsg
{
  MidiMsg() {}
  explicit constexpr MidiMsg(const uint8_t* raw) :
    s(raw[0] & 0xF0),
    c(raw[0] & 0x0F),
    d1(raw[1]), d2(raw[2])
    {}

  constexpr MidiMsg(const std::initializer_list<int>& raw)
  {
    uint8_t tmp[3];
    uint8_t i = 0;
    for (auto& x : raw)
      tmp[i++] = x;
    *this = MidiMsg(tmp);
  }

  uint8_t s = 0;
  uint8_t c = 0;
  uint8_t d1 = 0;
  uint8_t d2 = 0;

  std::vector<uint8_t> serialize() const
  {
    std::vector<uint8_t> res(3);
    res[0] = s | c;
    res[1] = d1;
    res[2] = d2;
    return res;
  }
};

struct MidiMsgHash
{
  constexpr uint64_t operator() (const MidiMsg& msg) const
  {
    return static_cast<uint32_t>(msg.s) << 24
        | static_cast<uint32_t>(msg.c) << 16
        | static_cast<uint32_t>(msg.d1) << 8;
        // | static_cast<uint32_t>(msg.d2) << 0;
  }
};

struct MidiMsgEquals
{
  constexpr uint64_t operator() (const MidiMsg& a, const MidiMsg& b) const
  {
    return MidiMsgHash()(a) == MidiMsgHash()(b);
  }
};
using MidiStack = std::vector<MidiMsg>;

using MidiCallback = std::function<void(const MidiMsg&)>;

using MidiHashMap = std::unordered_multimap<MidiMsg, MidiCallback, MidiMsgHash, MidiMsgEquals>;

template <typename F>
MidiCallback make_callback(const F& f)
{
  return [f](const MidiMsg&) -> std::vector<MidiMsg>
  {
    f();
    return std::vector<MidiMsg>();};
}