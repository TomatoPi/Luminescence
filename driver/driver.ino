#include "color_palette.h"

#include <FastLED.h>

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

using Range = range_t<index_t, coef_t>;
using Palette = color_pallette_t<color_t, index_t, coef_t>;

#include "palettes.h"

static constexpr index_t MaxLedsCount = 14;

Range pouet = Range::map_on_pixel_index(0, 14, 14, 0);
color_t leds[MaxLedsCount];

void setup()
{
  Serial.begin(9600);
  while (!Serial) ;
  FastLED.addLeds<NEOPIXEL, 2>(leds, MaxLedsCount);

  memset(leds, 32, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();
}

void loop()
{
  palette_rainbow.eval_range(leds, pouet);
  FastLED.show();
  delay(10);
}
