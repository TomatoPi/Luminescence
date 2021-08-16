#pragma once

/*
Warning :
  Code written in this file must be platform independant.
*/

#define START_BYTE 0x7f
#define STOP_BYTE 0xf7

struct SerialHeader
{
  const uint8_t bytes[4] = {0x0B, 0x0C, 0x0D, 0x0E};
};

template <typename SerialObj>
struct SerialParser
{
  SerialObj& obj = nullptr;

  SerialParser(SerialObj& obj) : obj(obj)
  {
    memset(&obj.obj, 0, obj.size);
  }

  enum ParsingResult{ Running = 0, Started = 1, Finished = 2 };
  ParsingResult parse(uint8_t byte)
  {
    if (obj._serial_index < sizeof(SerialHeader))
    {
      if (byte != obj._serial_header.bytes[obj._serial_index])
        return error(-10);

      obj._serial_index++;
      return Started;
    }

    obj._serial_buffer[obj._serial_index - sizeof(SerialHeader)] = byte;
    obj._serial_index++;

    if (size() <= obj._serial_index)
    {
      memcpy(obj.obj.raw, obj._serial_buffer, obj.size);
      obj._serial_index = 0;
      return Finished;
    }
    else
    {
      return Running;
    }
  }

  const uint8_t* serialize() const
  {
    return obj._serial_header.bytes;
  }

  const uint32_t size() const
  {
    return obj.size + sizeof(SerialHeader);
  }

  int error(int code)
  {
    obj._serial_index = 0;
    return code;
  }
};

#define Serializable(name, code) \
struct name##_splited_s { \
  code \
}; \
union name { \
  struct { code }; \
  uint8_t raw[sizeof(name##_splited_s)]; \
}

/*
  Padding added to this structure make it
  serializable as a null terminated byte stream
*/
#define MakeSerializable(name, Type) \
struct Type##_serial \
{ \
  static constexpr uint32_t size = sizeof(Type); \
  const uint8_t _padding[3] = {0, 0, 0}; \
  const SerialHeader _serial_header; \
  Type obj; \
  uint8_t _serial_buffer[sizeof(Type)] = {0}; \
  uint32_t _serial_index = 0; \
}; \
Type##_serial name; \
SerialParser<Type##_serial> name##_serializer(name);

Serializable(Color,
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t _;
);

Serializable(FrameGeneratorParams,
  Color plain_color;
);
