#include "color_palette.h"
#include "driver.h"

#include <FastLED.h>

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

using Range = range_t<index_t, coef_t>;
using Palette = color_pallette_t<color_t, index_t, coef_t>;

#include "palettes.h"

static constexpr index_t MaxLedsCount = 1;//30 * 39;
static constexpr index_t PaletteSize = 512;

coef_t master_clock = coef_t(0);
Range pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, 0);
color_t leds[MaxLedsCount];

MakeSerializable(optopoulpe, FrameGeneratorParams);

void setup()
{
  Serial.begin(9600);
  delay(1000);
  FastLED.addLeds<NEOPIXEL, 2>(leds, MaxLedsCount);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.setMaxRefreshRate(25, true);

  memset(leds, 0, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();

  Range tmp = Range::map_on_pixel_index(0, PaletteSize, PaletteSize, 0);

  Serial.println("Coucou");
  Serial.println(optopoulpe_serializer.size());
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
  static bool drop = false;

  while (0 < Serial.available()) { if (-1 == Serial.read()) break; }
  Serial.write(START_BYTE);
  Serial.flush();

  optopoulpe_serializer.error(0);

  bool frame_received = false;
  unsigned int timeout_cptr = 0;
  while (!frame_received)
  {
    if (Serial.available() <= 0)
    {
      ++timeout_cptr;
      if (10 < timeout_cptr)
      {
        Serial.write(START_BYTE);
        Serial.flush();
      }
      FastLED.delay(10);
      continue;
    }
    last_packet_timestamp = master_clock;
    
    int error = 0;
    int byte = Serial.read();
    
    if (byte < 0)
      continue;
    error = optopoulpe_serializer.parse(byte);

    Serial.print(millis());
    Serial.print(" : ");
    Serial.print(byte, HEX);
    Serial.print(" : idx : ");
    Serial.print((int)optopoulpe._serial_index);
    Serial.print(" : code : ");
    Serial.print((int)error);
    Serial.write(START_BYTE);

    switch (error)
    {
      case 0:
        break;
      case 1:
        message_begin_timestamp = master_clock;
        drop = false;
        break;
      case 2:
      {
        Serial.print(": full blob ok");
        const uint8_t* c = optopoulpe.obj.plain_color.raw;
        Serial.print("RGB : ");
        Serial.print(c[0], HEX); Serial.print(" ");
        Serial.print(c[1], HEX); Serial.print(" ");
        Serial.print(c[2], HEX);
        Serial.write(START_BYTE);
        frame_received = true;
        break;
      }
      default:
        Serial.print("error : ");
        Serial.print((int)error);
        Serial.write(START_BYTE);
        optopoulpe_serializer.error(0);
        while (0 < Serial.available()) { Serial.read(); } // trash upcoming datas
        Serial.write(START_BYTE);
        Serial.flush();
        FastLED.delay(10);
        break;
    }
  }

  if (!drop && 100 + 10 * 16 < master_clock - message_begin_timestamp)
  {
    drop = true;
    optopoulpe_serializer.error(-200);
    // Serial.println("drop");
  }
  
  if (false && 1000 < master_clock - last_packet_timestamp)
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
  FastLED.delay(100);// / 25);

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
