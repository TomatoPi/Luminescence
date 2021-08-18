#pragma once

#include <FastLED.h>

// From https://iquilezles.org/www/articles/palettes/palettes.htm

struct PaletteParams {
  uint8_t min_value;
  uint8_t max_value;
  uint8_t pulsation;
  uint8_t phase;

  uint8_t eval(uint8_t t) const;
};

struct ColorPalette 
{
  CRGB eval(uint8_t t) const;
  PaletteParams params_r;
  PaletteParams params_g;
  PaletteParams params_b;
};