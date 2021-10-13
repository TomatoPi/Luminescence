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

static_assert(sizeof(SerialPacket) == SerialPacket::Size);

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
    enum Object {
      Unknown  = 0,
      Setup,
      Master,
      Ribbon
    };
  }

  struct Setup
  {
    static constexpr const flags::Object Flag = flags::Object::Setup;
    //
    uint8_t ribbons_count;
    uint8_t ribbons_modules_count[8];
    // 9
  };

  struct Master {
    static constexpr const flags::Object Flag = flags::Object::Master;
    //
    uint16_t bpm;                 // should be replaced by fixed point integer
    uint8_t sync_correction;      // Masterclock phase offset [0 - 255]
    uint8_t brightness      : 7;  // Master brightness [0 - 255] TODO : modify scaling
    uint8_t _p4_            : 1;
    // 4
    uint8_t group0_master : 3;
    uint8_t group1_master : 3;
    uint8_t _p5_          : 2;  // padding
    uint8_t group2_master : 3;
    uint8_t _p6_          : 5;  // padding
    // 6
    /*
    Assign Master encoders here ??
    */
  };

  struct Ribbon {
    static constexpr const flags::Object Flag = flags::Object::Ribbon;
    //
    uint8_t index               : 4; // [0 - 8] 8 is master
    uint8_t group               : 2; // [0 - 2]
    uint8_t _p1_                : 2; // padding
    // 1
    uint8_t color_mod_raw_param1: 7; // @see selector encoder
    uint8_t color_mod_enable    : 1;
    uint8_t color_mod_intensity : 6;
    uint8_t color_mod_reverse   : 1;
    uint8_t color_mod_sw        : 1; // Up to decide
    // 3
    uint8_t light_mod_raw_param1: 7; // @see selector encoder
    uint8_t light_mod_enable    : 1;
    uint8_t light_mod_intensity : 6;
    uint8_t light_mod_reverse   : 1;
    uint8_t light_mod_is_dot    : 1;
    // 5
    uint8_t recurse_depth       : 3; // [0 - 3] or 4
    uint8_t recurse_enable      : 1;
    uint8_t _p6_                : 4; // padding
    uint8_t recurse_scale_ratio : 6;
    uint8_t recurse_scale_reverse : 1;
    uint8_t recurse_alternate   : 1;
    // 7
    uint8_t feedback_intensity  : 7;
    uint8_t feedback_enable     : 1;
    uint8_t _p9_                : 7; // Up to decide
    uint8_t feedback_fadein     : 1;
    // 9
    uint8_t brightness          : 7;
    uint8_t strobe              : 1;
  };
}
