#include "color_palette.h"
#include "palettes.h"
#include "clock.h"

#include "range.h"

#include "common.h"

#include <FastLED.h>

#include "apply_modulation.h"

/*
Motifs :

  Réglage influence temps :
    U [0, 1] : 0 inversé, 1 normal, 0.5 arret
    t <- mod1((2U - 1) * t)

  Modulation de l'onde :
    R : 

  Ondes linéaires :
    Modulation sur amplitude :

    Amplitude(t) = (1 - Range)()
*/

#define SERIAL_MESSAGE_TIMEOUT 30
#define SERIAL_SLEEP_TIMEOUT 5
#define FRAME_REFRESH_TIMEOUT 0

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

using Range = range_t<index_t, coef_t>;

// Needed with Arduino IDE;
constexpr const uint8_t SerialPacket::Header[3];
uint8_t Clock::_clockIndex = 0;
Clock* Clock::_clocks[Clock::MaxClocksCount];

static constexpr index_t MaxLedsCount = 30 * 20;

color_t leds[MaxLedsCount];

Clock master_clock;
Clock strobe_clock;

objects::Master master;
objects::Compo compos[8];

SerialParser parser;

void init_objects()
{
  master = {
    20,  // bpm
    0,    // sync  
    10,  // brigthness
    0
  };
  uint8_t idx = 0;
  for (auto& compo : compos)
  {
    compo = {
      idx++,
      0,
      { 0 }
    };
  }
  compos[0].modulation.kind = objects::flags::ModulationKind::SawTooth;
  compos[0].modulation.istimemod = 0;
  compos[0].modulation.min = 0;
  compos[0].modulation.max = 255;
  
  compos[1].modulation.kind = objects::flags::ModulationKind::SawTooth;
  compos[1].modulation.istimemod = 1;
  compos[1].modulation.min = 0;
  compos[1].modulation.max = 255;
}

/// Remaps i that is in the range [0, max_i-1] to the range [0, 255]
uint8_t map_to_0_255(uint32_t i, uint32_t max_i)
{
  uint32_t didx = 0xFFFFFFFFu / max_i;
  return static_cast<uint8_t>((i * didx) >> 24);
}

void setup()
{
  init_objects();
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, 2>(leds, MaxLedsCount);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.setMaxRefreshRate(50);

  memset(leds, 30, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();
  FastLED.delay(100);
  memset(leds, 0, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();

  Serial.println("Coucou");
  Serial.write(STOP_BYTE);
  FastLED.delay(1000);
}

void loop()
{
// Too slow...
//  master_clock = fmod(master_clock + coef_t(0.01f), coef_t(1.f));
//  pouet = Range::map_on_pixel_index(0, MaxLedsCount, MaxLedsCount, master_clock);
//  palette_rainbow.eval_range(leds, pouet);

// Cheaper prototype. Use integer arithmetics

  static unsigned long drop_count = 0;
  
  master_clock.setPeriod(1 + (60lu * 1000lu) / ((master.bpm + 1)));
  strobe_clock.setPeriod(master_clock.period >> master.strobe);

  Clock::Tick(millis());

  uint8_t time = master_clock.get8() + master.sync_correction;

  unsigned long update_begin = millis();
  drop_count += update_frame();
  unsigned long update_end = millis();

  unsigned long compute_begin = millis();

  for (index_t i = 0; i < MaxLedsCount; ++i) {
    uint8_t space = map_to_0_255(i, MaxLedsCount);
    uint8_t value = 0;
    for (const auto& compo : compos) {
     // value = apply_modulation(compo.modulation, value, time, space);
    }
    leds[i] = CRGB(255, 14, 50); //Palettes::rainbow.eval(value);
  }
  unsigned long compute_end = millis();
  
  unsigned long draw_begin = millis();
  if (master.strobe)
  {
    uint32_t pulse_width;
    switch (master.pulse_width)
    {
      case 3 : pulse_width = (0xFFFFFFFFu / 4u) * 3u; break;
      case 2 : pulse_width = 0xFFFFFFFFu / 2u; break;
      case 1 : pulse_width = 0xFFFFFFFFu / 3u; break;
      case 0 :
      default: pulse_width = 0xFFFFFFFFu / 10u; break;
    }

//    Serial.print(master.pulse_width);
//    Serial.print(" : PW : ");
//    Serial.print(pulse_width);
//    Serial.print(" : CK : ");
//    Serial.println(strobe_clock.clock);
//    Serial.write(STOP_BYTE);

    if (master.istimemod)
    {
      FastLED.show(strobe_clock.clock < pulse_width ? 0xFF : 0x00);
    }
    else // running pulses
    {
      /*
      Creates realy nice tracers on the ribbon.
      Could be extracted to apply it on any composition
      */
      const index_t pw_inpixels = ((uint64_t)pulse_width * MaxLedsCount) >> 32;
      const index_t ck_inpixels = ((uint64_t)strobe_clock.clock * MaxLedsCount) >> 32;
      if (MaxLedsCount < ck_inpixels + pw_inpixels)
      {
        // splited pulse
        const index_t split = (ck_inpixels + pw_inpixels) % MaxLedsCount;

        nscale8_video(
          leds, 
          split, 
          0xFF);
        nscale8_video(
          leds + split, 
          MaxLedsCount - pw_inpixels, 
          0x00);
        nscale8_video(
          leds + ck_inpixels, 
          pw_inpixels - split,
          0xFF);
      }
      else
      {
        nscale8_video(
          leds, 
          ck_inpixels, 
          0x00);
        nscale8_video(
          leds + ck_inpixels, 
          pw_inpixels, 
          0xFF);
        nscale8_video(
          leds + ck_inpixels + pw_inpixels, 
          MaxLedsCount - ck_inpixels - pw_inpixels, 
          0x00);
      }
      FastLED.show(master.brightness);
    }
  }
  else // no strobe
  {
    FastLED.show(master.brightness);
  }
  unsigned long draw_end = millis();


  unsigned long endtime = millis();
  
  static unsigned long fps_accumulator= 0;
  static unsigned long frame_cptr = 0;
  fps_accumulator += endtime - master_clock.last_timestamp;
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
    Serial.print(" : Drops : ");
    Serial.print(drop_count);
    Serial.print(" : BPM : ");
    Serial.print(master.bpm);
    Serial.println(" : Clocks : ");
    Serial.println(master.strobe);
    Serial.println(master.istimemod);
    Serial.println(master.pulse_width);
    Serial.println(master.unused);
    Serial.write(STOP_BYTE);
    fps_accumulator = 0;
    frame_cptr = 0;
  }
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
        }
        // Send ACK
        Serial.write(0);
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
