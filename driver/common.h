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
  static constexpr const uint8_t Header[3] = {0xEB, 0xEC, 0xED};

  uint8_t header[3] = {0xEB, 0xEC, 0xED};
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
    static_assert(sizeof(T) <= SerialPacket::ObjectSizeMax);

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
    enum ObjectKind {
      Unknown     = 0,
      Setup,
      Master,
      Composition,
      Oscilator,
      Modulation,
    };

    enum OscillatorKind {
      Sin = 0,
      Varislope,
      Noise,
      SawTooth,
      Square,
      Triangle,
    };

    enum Modulation {
      RebaseOnIndex = 0x00,
      RebaseOnTime = 0x01,
      RebaseTimeOnIndex = 0x02,
      RebaseIndexOnTime = 0x03,
    };

    enum Blend {
      Keep = 0x00,
      Replace = 0x01,
      Add = 0x02,
      Substract = 0x03,
      Min = 0x04,
      Max = 0x05,
    };
  }

  struct Setup
  {
    static constexpr const flags::ObjectKind Flag = flags::ObjectKind::Setup;
    //
    uint8_t driver_index : 2;
  };

  struct Oscilator {
    static constexpr const flags::ObjectKind Flag = flags::ObjectKind::Oscilator;
    //
    uint8_t index : 2;
    uint8_t kind : 4;   // ModulationKind
    uint8_t subdivide : 3; // [0-7] multiply clock by 2 ^ (subdivide - 5)
    uint8_t param1 : 7; // saturation | fixed_point | fixed_point
    uint8_t param2 : 7; // -- | 0 square, 127 triangle | variance
    // 2 + 2
  };

  struct Master {
    static constexpr const flags::ObjectKind Flag = flags::ObjectKind::Master;
    //
    uint16_t bpm;             // should be replaced by fixed point integer
    uint8_t sync_correction;  // Masterclock phase offset [0 - 255]
    uint8_t brightness;       // Master brightness [0 - 255] TODO : modify scaling
    // 4
    uint8_t strobe : 2;       // enable strobbing on half, quarter or eight pulse
    uint8_t istimemod : 1;    // see Modulation
    uint8_t pulse_width : 2;  // 0.10 0.33 0.5 0.75
    uint8_t unused : 3;
    // 5
    uint8_t active_compo : 4;
  };

  struct Compo {
    static constexpr const flags::ObjectKind Flag = flags::ObjectKind::Composition;
    //
    uint8_t index : 4;    // [0 - 8] 8 is master
    uint8_t palette : 4;  // [0 - 15]
    // 1
    uint8_t palette_width : 7;
    uint8_t _ : 1;
    uint8_t mod_intensity : 7;
    uint8_t __ : 1;
    uint8_t speed : 2;
    // 3
  };
}
