#include "color_palette.h"
#include "palettes.h"
#include "clock.h"
#include "range.h"
#include "common.h"
#include <FastLED.h>
#include "apply_modulation.h"
#include "math.h"
#include "Composition.h"

#define SERIAL_MESSAGE_TIMEOUT 30
#define SERIAL_SLEEP_TIMEOUT 5
#define FRAME_REFRESH_TIMEOUT 0

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

// Needed with Arduino IDE;
constexpr const uint8_t SerialPacket::Header[3];

index_t ribbonsCount = 4;
index_t Ribbons[MaxRibbonsCount] = {1, 1, 1, 1};

color_t leds[LedsCount];

FastClock strobe_clock;
Clock osc_clocks[4];
Clock& master_clock = osc_clocks[3];

objects::Master master;
objects::Compo compos[8];
objects::Oscilator oscillators[3];
objects::Sequencer sequencer;

FallDetector beat_detector(&master_clock);

SerialParser parser;

void setup()
{
  Serial.begin(115200);
  FastLED.addLeds<WS2811_PORTD, MaxRibbonsCount>(leds, LedsCount / MaxRibbonsCount);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.setMaxRefreshRate(50);

  memset(leds, 30, sizeof(CRGB) * LedsCount);
  FastLED.show();
  FastLED.delay(100);
  memset(leds, 0, sizeof(CRGB) * LedsCount);
  FastLED.show();

  Serial.write(STOP_BYTE);
  FastLED.delay(1000);
}

void update_clocks()
{
  master_clock.setPeriod(1 + (60lu * 1000lu) / ((/*master.bpm*/ 30 + 1)));
  Clock::Tick(millis());
  FastClock::Tick();
  FallDetector::Tick();
}

void loop()
{
    const auto& palette = Palettes::rainbow;
    const Composition compo{
        PaletteRangeController {
            OscillatorKind::Sin,
            255
        }
    };
    update_clocks();
    const uint8_t time = master_clock.get8() + master.sync_correction;
    memset(leds, 0, sizeof(CRGB) * LedsCount);
    for (uint32_t i = 0; i < LedsCount; ++i) {
        leds[i] = compo.eval(palette, time, i);
    }
    FastLED.show();
    FastLED.delay(20);
}

int update_frame() {
  
  Serial.write(STOP_BYTE);
  Serial.flush();
  delay(SERIAL_SLEEP_TIMEOUT);

  parser.error(0);
  
  int drop_count = 0;
  bool frame_received = false;
  unsigned long timeout_timestamp = millis();
  while (!frame_received)
  {
    if (Serial.available() <= 0)
    {
      if (SERIAL_MESSAGE_TIMEOUT < millis() - timeout_timestamp)
      {
        Serial.write(STOP_BYTE);
        Serial.flush();
        delay(SERIAL_SLEEP_TIMEOUT);
        timeout_timestamp = millis();
        parser.error(0);
        drop_count++;
      }
//      FastLED.delay(1);
      continue;
    }
    
    int byte = Serial.read();
    
    if (byte < 0)
      continue;
    ParsingResult result = parser.parse(byte);

//    Serial.print(byte, HEX);
//    Serial.print(" : idx : ");
//    Serial.print((int)parser.serial_index);
//    Serial.print(" : code : ");
//    Serial.print((int)result.status);
//    Serial.write(STOP_BYTE);
//    delay(SERIAL_SLEEP_TIMEOUT);

    switch (result.status)
    {
      case ParsingResult::Status::Running:
        break;
      case ParsingResult::Status::Started:
        // drop = false;
        break;
      case ParsingResult::Status::Finished:
      {
//        Serial.print(millis());
//        Serial.print(": full blob ok");
//        Serial.write(STOP_BYTE);

//        const uint8_t* c = parser.serial_buffer_in.rawobj;
//        Serial.print("RGB : ");
//        Serial.print(c[0], HEX); Serial.print(" ");
//        Serial.print(c[1], HEX); Serial.print(" ");
//        Serial.print(c[2], HEX); Serial.print(" ");
//        Serial.print(c[3], HEX);
//        Serial.write(STOP_BYTE);
        switch (result.flags)
        {
          case objects::flags::Master:
            master = result.read<objects::Master>();
            break;
          case objects::flags::Composition:
          {
            objects::Compo tmp = result.read<objects::Compo>();
            compos[tmp.index] = tmp;
            break;
          }
          case objects::flags::Sequencer:
          {
            sequencer = result.read<objects::Sequencer>();
            break;
          }
          case objects::flags::Oscilator:
          {
            auto tmp = result.read<objects::Oscilator>();
            oscillators[tmp.index] = tmp;
          }
        }
        // Send ACK
        char zero = '\0';
        Serial.write(&zero);
        Serial.write(STOP_BYTE);
        break;
      }
      case ParsingResult::Status::EndOfStream:
        frame_received = true;
        break;
      default:
//        Serial.print("error : ");
//        Serial.print((int)error);
//        Serial.write(STOP_BYTE);
        parser.error(0);
        //while (-1 != Serial.read());
        Serial.write(STOP_BYTE);
        Serial.flush();
        delay(SERIAL_SLEEP_TIMEOUT);
        timeout_timestamp = millis();
        drop_count++;
        break;
    }
  }
  return drop_count;
}
