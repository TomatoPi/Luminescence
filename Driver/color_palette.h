#pragma once

#include "state.h"

#include <FastLED.h>

// From https://iquilezles.org/www/articles/palettes/palettes.htm

using ColorPalette = state_t::palette_t;

inline ColorPalette lerp_palette(const ColorPalette& A, const ColorPalette& B, uint8_t t)
{
  ColorPalette res;
  for (size_t i=0 ; i<3 ; ++i)
  {
    res.params[i].min_value = lerp8by8(A.params[i].min_value, B.params[i].min_value, t);
    res.params[i].max_value = lerp8by8(A.params[i].max_value, B.params[i].max_value, t);
    res.params[i].frequency_times_60 = lerp8by8(A.params[i].frequency_times_60, B.params[i].frequency_times_60, t);
    res.params[i].phase = lerp8by8(A.params[i].phase, B.params[i].phase, t);
  }
  return res;
}

inline uint8_t eval(const ColorPalette::params_t& p, uint8_t t)
{
  return dim8_video(map8(cos8(p.frequency_times_60 * t / 60 + p.phase), p.min_value, p.max_value));
}
inline CRGB eval(const ColorPalette& p, uint8_t t)
{
  return { eval(p.params[0], t), eval(p.params[1], t), eval(p.params[2], t) };
}
