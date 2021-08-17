#pragma once

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

  const uint8_t header[3] = {0xEB, 0xEC, 0xED};
  uint8_t flags = 0;
  uint8_t rawobj[12] = {0};

  void clear() { flags = 0; for (uint8_t i=0; i<ObjectSizeMax;++i) rawobj[i] = 0; }
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

      if (byte != serial_buffer_in.header[serial_index])
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

struct Serializer
{
  SerialPacket serial_buffer_out;

  template <typename T>
  const uint8_t* serialize(const T& obj, uint8_t flags = 0)
  {
    static_assert(sizeof(T) <= SerialPacket::ObjectSizeMax);

    serial_buffer_out.clear();
    const uint8_t* raw = (uint8_t*)&obj;
    for (uint8_t i = 0; i < sizeof(T) ; ++i)
      serial_buffer_out.rawobj[i] = raw[i];
    serial_buffer_out.flags = T::Flag | flags;

    return (uint8_t*)&serial_buffer_out;
  }
};

/////////////////////////////////////////////////////////
// Available packets
/////////////////////////////////////////////////////////

enum Flags {
  ObjectRGB = 0x01,
};

namespace objects
{
  struct RGB {
    static constexpr const Flags Flag = Flags::ObjectRGB;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t x;
    uint16_t bpm;
    uint8_t sync_correction;
  };
}
