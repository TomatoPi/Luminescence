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

static constexpr index_t MaxLedsCount = 30 * 21;

#include "math.h"
color_t leds[MaxLedsCount];

FastClock strobe_clock;
Clock osc_clocks[4];
Clock& master_clock = osc_clocks[3];

objects::Master master;
objects::Compo compos[8];
objects::Oscilator oscillators[3];
objects::Sequencer sequencer;

FallDetector beat_detector(&master_clock);

SerialParser parser;

uint8_t eval_oscillator(const objects::Oscilator& osc, uint8_t speed, uint8_t time)
{
  uint8_t value = time;
  switch (osc.source)
  {
    // TODO : we need to introduce poulpe structure before
  }
  using namespace objects::flags;
  switch (osc.kind)
  {    
    case OscillatorKind::Sin:      return sin8(value);
    case OscillatorKind::SawTooth: return value;
    case OscillatorKind::Square:   return value < osc.param1 ? 0 : 255; break;
    case OscillatorKind::Triangle: return triwave8(value); break;
    case OscillatorKind::Noise:    return sin8(random8(value)); break;
    default:
      return 0;
  }
  return value;
}
uint8_t eval_oscillator(const objects::Oscilator& osc, uint8_t speed = 0)
{
  return eval_oscillator(osc, speed, osc_clocks[osc.index].get8(speed));
}

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
      0
    };
  }
}

/// Remaps i that is in the range [0, max_i-1] to the range [0, 255]
uint8_t map_to_0_255(uint32_t i, uint32_t max_i)
{
  uint32_t didx = 0xFFFFFFFFu / max_i;
  return static_cast<uint8_t>((i * didx) >> 24);
}

void eval_range(const objects::Compo& compo, index_t begin, index_t end)
{
  if (end <= begin)
    return;

  const auto& palette = Palettes::Get(compo.palette);
  const index_t length = end - begin;

  uint8_t time_mod = scale8(eval_oscillator(oscillators[0], compo.speed), compo.mod_intensity);
  
  uint16_t p_value = time_mod << 8;

  const uint16_t pixel_dt = compo.map_on_index ? (0xFFFFu / length) : 0;
  const uint16_t step = scale16by8(pixel_dt, compo.palette_width << 1);

  p_value += scale16by8(0xFFFFu, compo.index_offset << 1);
  p_value += pixel_dt * begin;

//  Serial.println();
//  Serial.print(compo.map_on_index);
//  Serial.print(" ");
//  Serial.print(compo.effect1);
//  Serial.print(" ");
//  Serial.print(compo.blend_mask);
//  Serial.print(" ");
//  Serial.print(compo.stars);
//  Serial.print(" ");
//  Serial.println(compo.strobe);
//  Serial.write(STOP_BYTE);

  // {
  //   for (index_t i = begin; i < end; ++i, p_value += step)
  //   {
  //     const uint8_t ribbon = (i * 255) / 20;
  //     uint8_t value = p_value >> 8;
  //     uint8_t bright = random8() < (compo.param_stars << 1) ? 0xFF : 0x00;
      
  //     if (compo.blend_mask)
  //       bright = lerp8by8(255, eval_oscillator(oscillators[0]), compo.blend_overlay << 1);
  //     leds[i] = nblend(leds[i], palette.eval(value), bright);
  //   }
  // }
  if (compo.strobe)
  {
    CRGB* leds = FastLED.leds() + begin;
    index_t leds_count = end - begin;
    uint32_t pulse_width;

    switch (master.pulse_width)
    {
      case 3 : pulse_width = (0xFFFFFFFFu / 4u) * 3u; break;
      case 2 : pulse_width = 0xFFFFFFFFu / 2u; break;
      case 1 : pulse_width = 0xFFFFFFFFu / 3u; break;
      case 0 :
      default: pulse_width = 0xFFFFFFFFu / 10u; break;
    }

    if (master.istimemod)
    {
      if (strobe_clock.coarse_value)
        for (index_t i = begin; i < end; ++i, p_value += step)
        {
          uint8_t value = p_value >> 8;
          leds[i] = palette.eval(value);
        }
      else {}
        //fill_solid(leds + begin, end - begin, CRGB::Black);
    }
    else // running pulses
    {
      /*
      Creates realy nice tracers on the ribbon.
      Could be extracted to apply it on any composition
      */
      const index_t pw_inpixels = ((uint64_t)pulse_width * length) >> 32;
      const index_t ck_inpixels = ((uint64_t)strobe_clock.finevalue * length) >> 8;
      if (length < ck_inpixels + pw_inpixels)
      {
        // splited pulse
        const index_t split = (ck_inpixels + pw_inpixels) % length;

        index_t i = 0;

        for (; i < split ; ++i, p_value += step)
        {
          uint8_t value = p_value >> 8;
          leds[begin+i] = palette.eval(value);
        }

        for (; i < split + length - ck_inpixels ; ++i, p_value += step)
        {
          //leds[begin+i] = CRGB::Black;
        }

        for (; i < length ; ++i, p_value += step)
        {
          uint8_t value = p_value >> 8;
          leds[begin+i] = palette.eval(value);
        }
      }
      else
      {
        index_t i = 0;
        for (; i < ck_inpixels ; ++i, p_value += step)
        {
          //leds[begin+i] = CRGB::Black;
        }
        for (; i < ck_inpixels + pw_inpixels ; ++i, p_value += step)
        {
          uint8_t value = p_value >> 8;
          leds[begin+i] = palette.eval(value);
        }
        for (; i < length ; ++i, p_value += step)
        {
          //leds[begin+i] = CRGB::Black;
        }
      }
    } // endif running pulses
  } // endif strobe
  else // normal
  {
    for (index_t i = begin; i < end; ++i, p_value += step)
    {
      const uint8_t ribbon = (i * 255) / 20;
      uint8_t value = p_value >> 8;
      uint8_t bright = 255;

      if (compo.stars)
        bright = random8() < (compo.param_stars << 1) ? 0xFF : 0x00;
      else
        bright = compo.brightness;

      if (compo.blend_mask)
        bright = lerp8by8(bright, eval_oscillator(oscillators[0]), compo.blend_overlay << 1);

      if (compo.effect1)
        bright = scale8(value, 255 - compo.param1);

      leds[i] = nblend(leds[i], palette.eval(value), bright);
    }
  }
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
  strobe_clock.setPeriod(master.strobe_speed);

  for (uint8_t i = 0 ; i < 3 ; ++i)
  {
    uint8_t subdivide = oscillators[i].subdivide;
    uint32_t period = master_clock.period;
    if (subdivide < 5)
      period = period >> (5 - subdivide);
    else
      period = period << subdivide;
    osc_clocks[i].setPeriod(period);
  }

  Clock::Tick(millis());
  FastClock::Tick();
  FallDetector::Tick();

  uint8_t time8 = master_clock.get8() + master.sync_correction;

  unsigned long update_begin = millis();
  drop_count += update_frame();
  unsigned long update_end = millis();

  unsigned long compute_begin = millis();

  bool set_black = true;
  if (master.fade)
  {
    fadeLightBy(leds, MaxLedsCount, 255 - (master.feedback << 1));
    set_black = false;
  }
  if (master.blur)
  {
    blur1d(leds, MaxLedsCount, master.feedback << 1);
    set_black = true;
  }
  if (set_black)
  {
    fill_solid(leds, MaxLedsCount, CRGB::Black);
  }

  if (8 <= master.active_compo)
  {
    static uint8_t current_step = 0;
    
    if (beat_detector.trigger)
    {
      current_step = (current_step +1) % 3;
      if (!sequencer.steps[current_step]) current_step = (current_step +1) % 3;
      if (!sequencer.steps[current_step]) current_step = (current_step +1) % 3;
      beat_detector.reset();
    }
    Serial.println(sequencer.steps[current_step]);
    Serial.println(current_step);
    Serial.write(STOP_BYTE);
    for (uint8_t i = 0 ; i < 8 ; ++i)
      if (sequencer.steps[current_step] & (1 << i))
        eval_range(compos[i], 0, MaxLedsCount);
  }
  else
  {
    eval_range(compos[master.active_compo], 0, MaxLedsCount); 
  }
  unsigned long compute_end = millis();
  
  unsigned long draw_begin = millis();
  if (master.do_strobe)
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
      FastLED.show(strobe_clock.coarse_value ? 0xFF : 0x00);
    }
    else // running pulses
    {
      /*
      Creates realy nice tracers on the ribbon.
      Could be extracted to apply it on any composition
      */
      const index_t pw_inpixels = ((uint64_t)pulse_width * MaxLedsCount) >> 32;
      const index_t ck_inpixels = ((uint64_t)strobe_clock.finevalue * MaxLedsCount) >> 8;
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
    Serial.println(master.bpm);
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
