#pragma once

#include <FastLED.h>

// From https://iquilezles.org/www/articles/palettes/palettes.htm

struct ColorPalette 
{
  CRGB eval(uint8_t t) const;
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