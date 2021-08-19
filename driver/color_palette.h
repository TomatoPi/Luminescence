#pragma once

#include <FastLED.h>

// From https://iquilezles.org/www/articles/palettes/palettes.htm

struct PaletteParams {
  uint8_t min_value;
  uint8_t max_value;
  uint8_t pulsation;
  uint8_t phase;

  // Theses functions have to be inilined
  uint8_t eval(uint8_t t) const
  {
    return map8(cos8(pulsation * t + phase), min_value, max_value);
  }
};

struct ColorPalette 
{
  // Same
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