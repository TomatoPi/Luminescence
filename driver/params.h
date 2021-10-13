#pragma once

#include <stdint.h>

struct encoder {
  int8_t u = 0;
  uint8_t raw_value = 0; ///!< 7 bits encoder value
};

///
/// First encoder is divided in 5 zones
///   Both extremums (0 and 7F) are mapped to 4
///   Remaining range is equaly divided in 4 subranges
///     with mode from 0 to 3, 
///     and value from 0 to 32
///
struct selector_encoder : public encoder {
  int active_mode() const
  {
    if (0x00 == raw_value || 0x7F == raw_value)
      return 4;
    else
      return (raw_value - 2) >> (7 - 2); // 2^2 values
  }

  uint8_t value() const // 5bits subvalue
  {
    return (raw_value - 2) % (1 << (7 - 2));
  }
};

///
/// Second encoder is divided in two zones
///   On the left values goes from 0 to 64 anti-clockward
///   On the right values goes from 0 to 63 clockward
///   Center value is mapped to 0
///
struct panning_encoder : public encoder {
  int sign() const // -1 on the left, 0 on center, 1 on the right
  {
    if (raw_value < 64)
      return -1;
    else if (raw_value > 64)
      return 1;
    else
      return 0;
  }

  uint8_t value() const // 6bits value
  {
    switch (sign())
    {
      case -1 : return 64 - raw_value;
      case 1 : return raw_value - 64;
      case 0 : return 0;
    }
  }
};

///
/// Common structure for parameters with an assigned pot :
///   Color Modulation
///   Intensity Modulation
///   Recursion
///
struct modulator_control {
  selector_encoder selector;  ///!< Modulation LFO / Recursion depth
  panning_encoder intensity;  ///!< Modulation magnitude / Recursion split ratio
  bool toggle;                ///!< Toggle button below encoders

  bool enable;                ///!< Assigned to pad matrix
};

struct tracer_control {
  uint8_t feedback;
  uint8_t pot2;
  bool toogle;

  bool enable;
}