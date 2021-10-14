#pragma once

#include <stdint.h>

/*
Warning :
  Code written in this file must be platform independant.
*/

#define STOP_BYTE 0xF7

struct SerialPacket
{
  static constexpr const uint8_t HeaderSize = 3;
  static constexpr const uint8_t ObjectPacketOffset = 4;
  static constexpr const uint8_t ObjectSizeMax = 12;
  static constexpr const uint8_t Size = 16;
  static constexpr const uint8_t Header[3] = {0xEB, 0xEC, 0xED};

  uint8_t header[3] = {0xEB, 0xEC, 0xED};
  uint8_t flags = 0;
  uint8_t rawobj[12] = {0};

  void clear() { flags = 0; for (uint8_t i=0; i<ObjectSizeMax;++i) rawobj[i] = 0; }

  template <typename T>
  T read() const { return T(*(T*)rawobj); }
};

static_assert(sizeof(SerialPacket) == SerialPacket::Size, "Message");

struct ParsingResult {
  enum class Status {
    Running = 0,
    Started = 1,
    Finished = 2,
    EndOfStream = 3,
    Error = 4
  };
  Status status = Status::Running;
  uint8_t flags;
  uint8_t* rawobj;
  uint8_t error;

  ParsingResult(Status s, uint8_t f = 0, void* o = nullptr, uint8_t e = 0) : status(s), flags(f), rawobj((uint8_t*)o), error(e) {}

  static ParsingResult Started() { return {Status::Started, 0}; }
  static ParsingResult Running() { return {Status::Running, 0}; }
  static ParsingResult Finished(uint8_t flags, void* obj) { return {Status::Finished, flags, obj}; }
  static ParsingResult EndOfStream() { return {Status::EndOfStream}; }
  static ParsingResult Error(uint8_t err) { return {Status::Error, 0, 0, err}; }

  template <typename T>
  T read() const { return T(*(T*)rawobj); }
};

struct SerialParser
{
  SerialPacket serial_buffer_in;
  uint8_t serial_index = 0;

  ParsingResult parse(uint8_t byte)
  {
    if (serial_index < SerialPacket::HeaderSize)
    {
      if (0 == serial_index && static_cast<uint8_t>(STOP_BYTE) == byte)
        return ParsingResult::EndOfStream();

      if (byte != SerialPacket::Header[serial_index])
        return error(-10);

      serial_index++;
      return ParsingResult::Started();
    }

    serial_buffer_in.rawobj[serial_index - SerialPacket::ObjectPacketOffset] = byte; // will write in flags too
    serial_index++;

    if (SerialPacket::Size <= serial_index)
    {
      serial_index = 0;
      return ParsingResult::Finished(serial_buffer_in.flags, serial_buffer_in.rawobj);
    }
    else
    {
      return ParsingResult::Running();
    }
  }

  ParsingResult error(int code)
  {
    serial_buffer_in.clear();
    serial_index = 0;
    return ParsingResult::Error(code);
  }
};

namespace Serializer
{
  template <typename T>
  SerialPacket serialize(const T& obj, uint8_t flags = 0)
  {
    static_assert(sizeof(T) <= SerialPacket::ObjectSizeMax, "T must be smaller than packet size");

    SerialPacket packet;
    packet.clear();
    const uint8_t* raw = (uint8_t*)&obj;
    for (uint8_t i = 0; i < sizeof(T) ; ++i)
      packet.rawobj[i] = raw[i];
    packet.flags = T::Flag | flags;

    return packet;
  }
  inline const uint8_t* bytestream(const SerialPacket& packet)
  {
    return (uint8_t*)&packet;
  }
};

/////////////////////////////////////////////////////////
// Available packets
/////////////////////////////////////////////////////////

namespace objects
{

  namespace flags
  {
    enum Objects {
      Unknown     = 0,
      Setup,
      Master,
      Preset,
      Group,
      Ribbon
    };
  }

  struct Setup
  {
    static constexpr const flags::Objects Flag = flags::Objects::Setup;
    //
    uint8_t ribbons_count;
    uint8_t ribbons_lengths[8];
  };

  struct Master {
    static constexpr const flags::Objects Flag = flags::Objects::Master;
    //
    uint16_t bpm;                 // should be replaced by fixed point integer
    uint8_t sync_correction;      // Masterclock phase offset [0 - 255]
    uint8_t brightness      : 7;  // Master brightness [0 - 255] TODO : modify scaling
    uint8_t _p4_            : 1;
    // 4
    uint8_t encoders[8];
  };

  struct Preset {
    static constexpr const flags::Objects Flag = flags::Objects::Preset;
    
    uint8_t index : 3;
    uint8_t pads_states : 5;
    // 1
    uint8_t encoders[8];
    // 9
    uint8_t switches : 4;
    uint8_t __0__ : 4;
    // 10
    uint8_t brightness : 7;
    uint8_t __1__ : 1;
  };

  struct Group {
    static constexpr const flags::Objects Flag = flags::Objects::Group;

    uint8_t index : 2;
    uint8_t preset : 3;
    uint8_t palette : 3;
    // 1
    uint8_t palette_width : 7;
  };

  struct Ribbon {
    static constexpr const flags::Objects Flag = flags::Objects::Ribbon;

    uint8_t index : 4;
    uint8_t group : 2;
    uint8_t __0__ : 2;
    // 1
  };

  // struct Compo {
  //   static constexpr const flags::Objects Flag = flags::Objects::Composition;
  //   //
  //   uint8_t index : 4;    // [0 - 8] 8 is master
  //   uint8_t palette : 4;  // [0 - 15]
  //   // 1
  //   uint8_t palette_width : 7;
  //   uint8_t _ : 1;
  //   uint8_t mod_intensity : 7;
  //   uint8_t __ : 1;
  //   // 3
  //   uint8_t index_offset : 7;
  //   uint8_t map_on_index : 1;
  //   // 4
  //   uint8_t param1 : 7;
  //   uint8_t effect1 : 1; // Up to decide
  //   // 5
  //   uint8_t blend_overlay : 7;
  //   uint8_t blend_mask : 1; // Up to decide
  //   // 6
  //   uint8_t param_stars: 7; 
  //   uint8_t stars : 1;  // Chain break like a diamond
  //   // 7
  //   uint8_t speed : 2;
  //   uint8_t strobe : 1;
  //   uint8_t trigger : 1;
  //   uint8_t ___ : 4;
  //   // 8
  //   uint8_t brightness : 7;
  // };

  // struct Sequencer
  // {
  //   static constexpr const flags::Objects Flag = flags::Objects::Sequencer;
  //   uint8_t steps[3];
  // };
}
