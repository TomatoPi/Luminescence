#include "color_palette.h"
#include "driver.h"

#include <FastLED.h>

#define SERIAL_MESSAGE_TIMEOUT 30
#define SERIAL_SLEEP_TIMEOUT 5
#define FRAME_REFRESH_TIMEOUT 0

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

using Range = range_t<index_t, coef_t>;
using Palette = color_pallette_t<color_t, index_t, coef_t>;

#include "palettes.h"

static constexpr index_t MaxLedsCount = 30 * 20;
static constexpr index_t PaletteSize = 512;

coef_t master_clock = coef_t(0);
Range pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, 0);
color_t leds[MaxLedsCount];

MakeSerializable(optopoulpe, FrameGeneratorParams);

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, 2>(leds, MaxLedsCount);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.setMaxRefreshRate(50);

  memset(leds, 0, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();

  Range tmp = Range::map_on_pixel_index(0, PaletteSize, PaletteSize, 0);

  Serial.println("Coucou");
  Serial.println(optopoulpe_serializer.size());
  Serial.write(START_BYTE);
  FastLED.delay(1000);
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

  static unsigned long last_packet_timestamp = 0;
  static unsigned long message_begin_timestamp = 0;
  static bool drop = false;

  unsigned long update_begin = millis();
  update_frame();
  unsigned long update_end = millis();

  unsigned long compute_begin = millis();
  const uint8_t* c = optopoulpe.obj.plain_color.raw;
  for (index_t i = 0 ; i < MaxLedsCount ; ++i)
  {
    leds[i] = CRGB(c[0], c[1]+ sin8(value), c[2] + cos8(value + i));
  }
  unsigned long compute_end = millis();

  unsigned long draw_begin = millis();
  FastLED.show();
  //FastLED.delay(FRAME_REFRESH_TIMEOUT);
  unsigned long draw_end = millis();

  unsigned long endtime = millis();
  
  static unsigned long fps_accumulator= 0;
  static unsigned long frame_cptr = 0;
  fps_accumulator += endtime - master_clock;
  frame_cptr++;

  if (2000 < fps_accumulator)
  {
    float fps = float(frame_cptr) * 1000.f / fps_accumulator;
    Serial.print("Avg FPS : ");
    Serial.print(fps);
    Serial.print(" : Serial : ");
    Serial.print(update_end - update_begin);
    Serial.print(" : Compute : ");
    Serial.print(compute_end - compute_begin);
    Serial.print(" : Draw : ");
    Serial.print(draw_end - draw_begin);
    Serial.write(START_BYTE);
    fps_accumulator = 0;
    frame_cptr = 0;
  }
}

void update_frame() {

  //while (-1 != Serial.read());
  
  Serial.write(START_BYTE);
  Serial.flush();
  delay(SERIAL_SLEEP_TIMEOUT);

  optopoulpe_serializer.error(0);

  bool frame_received = false;
  unsigned long timeout_timestamp = millis();
  while (!frame_received)
  {
    if (Serial.available() <= 0)
    {
      if (SERIAL_MESSAGE_TIMEOUT < millis() - timeout_timestamp)
      {
        Serial.write(START_BYTE);
        Serial.flush();
        delay(SERIAL_SLEEP_TIMEOUT);
        timeout_timestamp = millis();
        optopoulpe_serializer.error(0);
      }
//      FastLED.delay(1);
      continue;
    }
    
    int error = 0;
    int byte = Serial.read();
    
    if (byte < 0)
      continue;
    error = optopoulpe_serializer.parse(byte);

//    Serial.print(millis());
//    Serial.print(" : ");
//    Serial.print(byte, HEX);
//    Serial.print(" : idx : ");
//    Serial.print((int)optopoulpe._serial_index);
//    Serial.print(" : code : ");
//    Serial.print((int)error);
//    Serial.write(START_BYTE);

    switch (error)
    {
      case 0:
        break;
      case 1:
        // drop = false;
        break;
      case 2:
      {
//        Serial.print(millis());
//        Serial.print(": full blob ok");
//        Serial.write(START_BYTE);

//        const uint8_t* c = optopoulpe.obj.plain_color.raw;
//        Serial.print("RGB : ");
//        Serial.print(c[0], HEX); Serial.print(" ");
//        Serial.print(c[1], HEX); Serial.print(" ");
//        Serial.print(c[2], HEX);
//        Serial.write(START_BYTE);
        frame_received = true;
        break;
      }
      default:
//        Serial.print("error : ");
//        Serial.print((int)error);
//        Serial.write(START_BYTE);
        optopoulpe_serializer.error(0);
        //while (-1 != Serial.read());
        Serial.write(START_BYTE);
        Serial.flush();
        delay(SERIAL_SLEEP_TIMEOUT);
        timeout_timestamp = millis();
        break;
    }
  }
}
