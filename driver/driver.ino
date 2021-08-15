#include "color_palette.h"

#include <FastLED.h>

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

using Range = range_t<index_t, coef_t>;
using Palette = color_pallette_t<color_t, index_t, coef_t>;

#include "palettes.h"

static constexpr index_t MaxLedsCount = 30 * 21;

coef_t master_clock = coef_t(0);
Range pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, 0);
color_t leds[MaxLedsCount];

void setup()
{
  Serial.begin(9600);
  while (!Serial) ;
  FastLED.addLeds<NEOPIXEL, 2>(leds, MaxLedsCount);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);

  memset(leds, 32, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();
}

void loop()
{
// Too slow...
//  master_clock = fmod(master_clock + coef_t(0.01f), coef_t(1.f));
//  pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, master_clock);
//  palette_rainbow.eval_range(leds, pouet);

// Cheaper prototype. Use integer arithmetics
  static unsigned long master_clock = 0;
  static unsigned long master_clock_period = 2000;
  master_clock = millis();
  uint8_t value = ((master_clock % master_clock_period) * 255) / master_clock_period;
  for (unsigned int i = 0 ; i < MaxLedsCount ; ++i)
  {
    leds[i] = CRGB(
      value,
      value + 64, // No need for modulo thanks to overflow
      value + i   // Create nice tracers
    );
  }
  FastLED.show();
  delay(10);
}
