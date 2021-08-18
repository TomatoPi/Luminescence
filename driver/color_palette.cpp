#include "color_palette.h"

static uint8_t channel(uint8_t t, uint8_t min_value, uint8_t max_value, uint8_t pulsation, uint8_t phase) 
{
  return map8(cos8(pulsation * t + phase), min_value, max_value);
}

CRGB ColorPalette::eval(uint8_t t) const
{
  return {
    channel(t, min_value_r, max_value_r, pulsation_r, phase_r),
    channel(t, min_value_g, max_value_g, pulsation_g, phase_g),
    channel(t, min_value_b, max_value_b, pulsation_b, phase_b)
  };
}