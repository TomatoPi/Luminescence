#include "color_palette.h"

uint8_t PaletteParams::eval(uint8_t t) const
{
  return map8(cos8(pulsation * t + phase), min_value, max_value);
}

CRGB ColorPalette::eval(uint8_t t) const
{
  return {
    params_r.eval(t),
    params_g.eval(t),
    params_b.eval(t)
  };
}