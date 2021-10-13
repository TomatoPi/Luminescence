#pragma once

#include <FastLED.h>

// From https://iquilezles.org/www/articles/palettes/palettes.htm

struct PaletteParams {
  uint8_t min_value;
  uint8_t max_value;
  uint8_t frequency_times_60; // Allows us to have fractional frequencies like 1/60, 1/2 etc.
  uint8_t phase;

  // Theses functions have to be inlined
  uint8_t eval(uint8_t t) const
  {
    return map8(cos8(frequency_times_60 * t / 60 + phase), min_value, max_value);
  }
};

struct ColorPalette 
{
  // Theses functions have to be inlined
  CRGB eval(uint8_t t) const
  {
  return {
      params_r.eval(t),
      params_g.eval(t),
      params_b.eval(t)
    };
  }
  
  PaletteParams params_r;
  PaletteParams params_g;
  PaletteParams params_b;
};