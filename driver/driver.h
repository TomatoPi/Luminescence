#pragma once

/*
Warning :
  Code written in this file must be platform independant.
*/

#define START_BYTE 0xff
#define STOP_BYTE 0xff

template <typename SerialObj>
struct SerialParser
{
  SerialObj& obj = nullptr;
  enum Status {
    Ok, ErrWaitStop, ErrWaitStart
  } status;
  SerialParser(SerialObj& obj) : obj(obj), status(ErrWaitStart)
  {
    memset(&obj.obj, 0, obj.size);
  }

  int parse(uint8_t byte)
  {
    if (ErrWaitStop == status)
    {
      if (byte == STOP_BYTE)
      {
        status = ErrWaitStart;
        return 0;
      }
      return error(ErrWaitStop) - 10;
    }
    else if (ErrWaitStart == status)
    {
      if (byte != START_BYTE)
        return error(ErrWaitStart) - 20;
    }

    if (0 == obj._serial_index)
    {
      if (byte != START_BYTE)
        return error(ErrWaitStart) - 30;
      return 0;
    }
    else if (obj.size < obj._serial_index)
    {
      if (byte != STOP_BYTE)
        return error(ErrWaitStop);
      memcpy(&obj.obj, obj._serial_buffer[0], obj.size);
      obj._serial_index = 0;
      return 1;
    }
    else
    {
      obj._serial_buffer[obj._serial_index] = byte;
      obj._serial_index++;
      return 0;
    }
  }

  const uint8_t* serialize() const
  {
    return &obj._start_byte;
  }

  const uint32_t size() const
  {
    return obj.size + 2;
  }

  int error(Status err)
  {
    obj._serial_index = 0;
    status = err;
    return -err;
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
  const uint8_t _start_byte = START_BYTE; \
  Type obj; \
  const uint8_t _stop_byte = STOP_BYTE; \
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

