#pragma once

#include <FastLED.h>

// From https://iquilezles.org/www/articles/palettes/palettes.htm

static uint8_t channel(uint8_t t, uint8_t a, uint8_t b, uint8_t c, uint8_t d) 
{
  return a + map8(cos8(c * t + d), 0, b);
}

struct ColorPalette 
{
  inline CRGB eval(uint8_t t) const
  {
    return {
      channel(t, a_r, b_r, c_r, d_r),
      channel(t, a_g, b_g, c_g, d_g),
      channel(t, a_b, b_b, c_b, d_b)
    };
  }
  uint8_t a_r;
  uint8_t a_g;
  uint8_t a_b;
  uint8_t b_r;
  uint8_t b_g;
  uint8_t b_b;
  uint8_t c_r;
  uint8_t c_g;
  uint8_t c_b;
  uint8_t d_r;
  uint8_t d_g;
  uint8_t d_b;
};