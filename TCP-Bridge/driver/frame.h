#pragma once

#include <array>

struct Pixel
{
  union {
    struct {
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;
    };
    uint8_t raw[3];
  };
};

struct Optopoulpe
{
  static constexpr const size_t PixelsCount = 30 * 21;
  using Frame = std::array<Pixel,PixelsCount+1>;

  Frame frame;
};

