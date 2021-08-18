#pragma once

#include <FastLED.h>

// From https://iquilezles.org/www/articles/palettes/palettes.htm

struct ColorPalette 
{
  CRGB eval(uint8_t t) const;
  uint8_t min_value_r;
  uint8_t min_value_g;
  uint8_t min_value_b;
  uint8_t max_value_r;
  uint8_t max_value_g;
  uint8_t max_value_b;
  uint8_t pulsation_r;
  uint8_t pulsation_g;
  uint8_t pulsation_b;
  uint8_t phase_r;
  uint8_t phase_g;
  uint8_t phase_b;
};