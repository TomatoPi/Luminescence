#pragma once

/*
Warning :
  Code written in this file must be platform independant.
*/

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
  Type obj; \
  const uint32_t _padding = 0; \
  uint8_t buffer[sizeof(Type)]; \
  uint32_t serial_index; \
}; \
Type##_serial name;

Serializable(Color,
  uint8_t red;
  uint8_t green;
  uint8_t blue;
);

Serializable(FrameGeneratorParams,
  Color plain_color;
);

