#include "color_palette.h"

#include <FastLED.h>

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

using Range = range_t<index_t, coef_t>;
using Palette = color_pallette_t<color_t, index_t, coef_t>;

#include "palettes.h"

static constexpr index_t MaxLedsCount = 30 * 21;
static constexpr index_t PaletteSize = 512;

coef_t master_clock = coef_t(0);
Range pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, 0);
color_t leds[MaxLedsCount];
color_t palette[PaletteSize];

void setup()
{
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, 2>(leds, MaxLedsCount);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);

  memset(leds, 0, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();

  Range tmp = Range::map_on_pixel_index(0, PaletteSize, PaletteSize, 0);
  palette_rainbow.eval_range(palette, tmp);
}

void loop()
{
// Too slow...
//  master_clock = fmod(master_clock + coef_t(0.01f), coef_t(1.f));
//  pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, master_clock);
//  palette_rainbow.eval_range(leds, pouet);

// Cheaper prototype. Use integer arithmetics

  static unsigned long master_clock = 0;
  static unsigned long master_clock_period = 4000;
  master_clock = millis();
//  uint8_t value = ((master_clock % master_clock_period) * 255) / master_clock_period;

  unsigned long lastPacket = 0;

  if (lastPacket < 1000)
  {
    for (unsigned int i = 0 ; i < MaxLedsCount ; ++i)
      leds[i] = CRGB(0, 0+i, 127);
  }
  else
  {
    uint8_t r = 0 < 127 ? 255 : 0;
    for (unsigned int i = 0 ; i < MaxLedsCount ; ++i)
    {
      leds[i] = CRGB(r, 0, 0);
    }
  }
  FastLED.show();
  delay(0);

  unsigned long endtime = millis();
  
  static unsigned long fps_accumulator= 0;
  static unsigned long frame_cptr = 0;
  fps_accumulator += endtime - master_clock;
  frame_cptr++;

  if (2000 < fps_accumulator)
  {
    float fps = float(frame_cptr) * 1000.f / fps_accumulator;
    Serial.print("Avg FPS : ");
    Serial.println(fps);
    fps_accumulator = 0;
    frame_cptr = 0;
  }
}
