#include "color_palette.h"
#include "driver.h"

#include <FastLED.h>

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

using Range = range_t<index_t, coef_t>;
using Palette = color_pallette_t<color_t, index_t, coef_t>;

#include "palettes.h"

static constexpr index_t MaxLedsCount = 30 * 39;
static constexpr index_t PaletteSize = 512;

coef_t master_clock = coef_t(0);
Range pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, 0);
color_t leds[MaxLedsCount];

MakeSerializable(optopoulpe, FrameGeneratorParams);

void setup()
{
  Serial.begin(9600);
  FastLED.addLeds<NEOPIXEL, 2>(leds, MaxLedsCount);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);

  memset(leds, 0, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();

  Range tmp = Range::map_on_pixel_index(0, PaletteSize, PaletteSize, 0);

  Serial.println("Coucou");
  Serial.println(optopoulpe.size);
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
  uint8_t value = ((master_clock % master_clock_period) * 255) / master_clock_period;

  static unsigned long last_packet_timestamp = 0;
  static unsigned long message_begin_timestamp = 0;
  
  while (Serial.available())
  {
    int error = 0;
    int byte = Serial.read();
    Serial.print(byte);
    Serial.print(" ");
    
    if (byte < 0)
      error = optopoulpe_serializer.error(0);
    error = optopoulpe_serializer.parse(byte);

    if (0 < error && 0 == optopoulpe._serial_index)
    {
      Serial.println(": full blob ok");
    }

    if (error)
    {
      Serial.print("error : ");
      Serial.println(error);
    }
    
    last_packet_timestamp = master_clock;
  }
  
  if (1000 < master_clock - last_packet_timestamp)
  {
    uint8_t r = value < 127 ? 255 : 0;
    for (index_t i = 0 ; i < MaxLedsCount ; ++i)
    {
      leds[i] = CRGB(r, 0, 0);
    }
  }
  else
  {
    const uint8_t* c = optopoulpe.obj.plain_color.raw;
    for (index_t i = 0 ; i < MaxLedsCount ; ++i)
    {
      leds[i] = CRGB(c[0], c[1], c[2]);
    }
  }

  FastLED.show();
  Serial.flush();
  delay(1000 / 25);

  unsigned long endtime = millis();
  
  static unsigned long fps_accumulator= 0;
  static unsigned long frame_cptr = 0;
  fps_accumulator += endtime - master_clock;
  frame_cptr++;

  if (2000 < fps_accumulator)
  {
    float fps = float(frame_cptr) * 1000.f / fps_accumulator;
//    Serial.print("Avg FPS : ");
//    Serial.println(fps);
    fps_accumulator = 0;
    frame_cptr = 0;
  }
}
