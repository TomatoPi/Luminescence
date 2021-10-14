#include "color_palette.h"
#include "palettes.h"
#include "clock.h"
#include "range.h"
#include "common.h"
#include <FastLED.h>
#include "apply_modulation.h"
#include "math.h"
#include "Composition.h"
#include <array>

#define SerialUSB_MESSAGE_TIMEOUT 10
#define SerialUSB_SLEEP_TIMEOUT 0
#define FRAME_REFRESH_TIMEOUT 0

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

// Needed with Arduino IDE;
constexpr const uint8_t SerialPacket::Header[3];

color_t leds[LedsCount];

FastClock strobe_clock;
Clock osc_clocks[4];
Clock& master_clock = osc_clocks[3];

bool connection_lost = false;

struct
{
  objects::Setup setup;
  objects::Master master;
  std::array<objects::Preset, 8> presets;
  std::array<objects::Ribbon, 8> ribbons;
  std::array<objects::Group, 3> groups;
} Global;

FallDetector beat_detector(&master_clock);

SerialParser parser;

void setup()
{
  SerialUSB.begin(115200);
  FastLED.addLeds<WS2811_PORTD, MaxRibbonsCount>(leds, LedsCount / MaxRibbonsCount);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.setMaxRefreshRate(50);

  memset(leds, 30, sizeof(CRGB) * LedsCount);
  FastLED.show();
  FastLED.delay(100);
  memset(leds, 0, sizeof(CRGB) * LedsCount);
  FastLED.show();

  SerialUSB.write(STOP_BYTE);
  FastLED.delay(1000);
}

void update_clocks()
{
  master_clock.setPeriod(1 + ((60lu * 1000lu * 100lu) / ((Global.master.bpm + 1))) << 2);
  Clock::Tick(millis());
  FastClock::Tick();
  FallDetector::Tick();
}

void loop()
{
    // Receive new datas from SerialUSB
    static unsigned long drop_count = 0;
    
    unsigned long update_begin = millis();
    drop_count += read_from_controller();
    unsigned long update_end = millis();

    // Compute next frame  
    unsigned long compute_begin = millis();

    update_clocks();
    const uint8_t time = master_clock.get8() + Global.master.sync_correction;
    const auto& palette = Palettes::rainbow;
    const Composition compo{
        PaletteRangeController {
            OscillatorKind::SawTooth,
            255
        },
        Slicer {
            3,
            255,
            true,
            false
        },
        Mask {
          time,
            80,
            //80
        }
    };
    memset(leds, 0, sizeof(CRGB) * LedsCount);
    for (uint32_t i = 0; i < LedsCount; ++i) {
        leds[i] = compo.eval(palette, time, i, LedsCount);
    }
    
    unsigned long compute_end = millis();

    // Draw frame
    unsigned long draw_begin = millis();
    if (connection_lost)
      for (size_t i = 0 ; i < 30 ; ++i)
        leds[i] = master_clock.get8() < 0x7F ? CRGB::Red : CRGB::Black;
    FastLED.show(Global.master.brightness);
    delay(1);
    unsigned long draw_end = millis();


    unsigned long endtime = millis();

    // Compute FPS
    static unsigned long fps_accumulator= 0;
    static unsigned long frame_cptr = 0;
    fps_accumulator += endtime - master_clock.last_timestamp;
    frame_cptr++;

    // Send status message
    if (2000 < fps_accumulator)
    {
      float fps = float(frame_cptr) * 1000.f / fps_accumulator;
      SerialUSB.print("Avg FPS : ");
      SerialUSB.print(fps);
      SerialUSB.print(" : SerialUSB : ");
      SerialUSB.print(update_end - update_begin);
      SerialUSB.print(" : Compute : ");
      SerialUSB.print(compute_end - compute_begin);
      SerialUSB.print(" : Draw : ");
      SerialUSB.print(draw_end - draw_begin);
      SerialUSB.print(" : Drops : ");
      SerialUSB.print(drop_count);
      SerialUSB.print(" : Master Clock : ");
      SerialUSB.println(Global.master.bpm);
      SerialUSB.write(STOP_BYTE);
      fps_accumulator = 0;
      frame_cptr = 0;
    }
}

int read_from_controller() {
  
  SerialUSB.write(STOP_BYTE);
  SerialUSB.flush();
  
  parser.error(0);
  
  int drop_count = 0;
  bool frame_received = false;
  unsigned long timeout_timestamp = millis();
  while (!frame_received)
  {
    if (SerialUSB.available() <= 0)
    {
      if (SerialUSB_MESSAGE_TIMEOUT < millis() - timeout_timestamp)
      {
        connection_lost = true;
        return 0;
        SerialUSB.write(STOP_BYTE);
        SerialUSB.flush();
        delay(SerialUSB_SLEEP_TIMEOUT);
        timeout_timestamp = millis();
        parser.error(0);
        drop_count++;
      }
      continue;
    }

    connection_lost = false;
    int byte = SerialUSB.read();
    
    if (byte < 0)
      continue;
    ParsingResult result = parser.parse(byte);

    if (result.status != ParsingResult::Status::EndOfStream)
    {
  //    SerialUSB.print(byte, HEX);
  //    SerialUSB.print(" : idx : ");
  //    SerialUSB.print((int)parser.serial_index);
  //    SerialUSB.print(" : code : ");
  //    SerialUSB.print((int)result.status);
  //    SerialUSB.println();
  //    SerialUSB.write(STOP_BYTE);
  //    delay(SerialUSB_SLEEP_TIMEOUT);
    }

    switch (result.status)
    {
      case ParsingResult::Status::Running:
        break;
      case ParsingResult::Status::Started:
        // drop = false;
        break;
      case ParsingResult::Status::Finished:
      {
        SerialUSB.print(millis());
        SerialUSB.println(": full blob ok");
        SerialUSB.write(STOP_BYTE);

        const uint8_t* c = parser.serial_buffer_in.rawobj;
//        SerialUSB.print("RGB : ");
//        SerialUSB.print(c[0], HEX); SerialUSB.print(" ");
//        SerialUSB.print(c[1], HEX); SerialUSB.print(" ");
//        SerialUSB.print(c[2], HEX); SerialUSB.print(" ");
//        SerialUSB.print(c[3], HEX);
//        SerialUSB.println();
        switch (result.flags)
        {
          case objects::flags::Setup:
          {
            Global.setup = result.read<objects::Setup>();
            break;
          }
          case objects::flags::Master:
            Global.master = result.read<objects::Master>();
            break;
          case objects::flags::Preset:
          {
            objects::Preset tmp = result.read<objects::Preset>();
            Global.presets[tmp.index] = tmp;
            break;
          }
          case objects::flags::Group:
          {
            objects::Group tmp = result.read<objects::Group>();
            Global.groups[tmp.index] = tmp;
            break;
          }
          case objects::flags::Ribbon:
          {
            objects::Ribbon tmp = result.read<objects::Ribbon>();
            Global.ribbons[tmp.index] = tmp;
            break;
          }
        }
        // Send ACK
        SerialUSB.print('\0');
        SerialUSB.write(STOP_BYTE);
        break;
      }
      case ParsingResult::Status::EndOfStream:
        frame_received = true;
        break;
      default:
//        SerialUSB.print("error : ");
//        SerialUSB.print((int)result.error);
//        SerialUSB.println();
//        SerialUSB.write(STOP_BYTE);
        parser.error(0);
        //while (-1 != SerialUSB.read());
        SerialUSB.write(STOP_BYTE);
        SerialUSB.flush();
        delay(SerialUSB_SLEEP_TIMEOUT);
        timeout_timestamp = millis();
        drop_count++;
        break;
    }
  }
  return drop_count;
}
