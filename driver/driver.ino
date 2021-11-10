#include "color_palette.h"
#include "clock.h"
#include "range.h"
#include <FastLED.h>
#include "apply_modulation.h"
#include "math.h"
#include "Composition.h"
#include "state.h"
#include <array>

// WS2811_PORTD: 25,26,27,28,14,15,29,11

/*
 * TODO :
 * 
 * Fix color blending when computing leds colors
 * Fix colopalettes
 * Add continuous blending between palettes instead of hard transitions
 * 
 * Reintroduce sync correction
 * Evaluate time with 16 bits instead of 8
 * 
 * Add a samplehold control (wery usefull to sync noise)
 * Reimplement the speed_scale for oscillators
 * Prevent clocks from going back to zero when changing speed
 * 
 * Add master effects :
 *  - Blur post processing << doesn't work ??
 *  - Global sequencer for playing between ribbons
 *  
 *  Rework colormodulation controls ??
 */

#define SerialUSB_MESSAGE_TIMEOUT 1
#define SerialUSB_SLEEP_TIMEOUT 0
#define FRAME_REFRESH_TIMEOUT 0

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

color_t leds[MaxLedsCount];

Clock master_clock;
Clock osc_clocks[PRESETS_COUNT];
FallDetector beat_detectors[PRESETS_COUNT];
uint8_t holded_values[PRESETS_COUNT][MaxRibbonsCount][2];

FastClock strobe_clock;
uint8_t coarse_framerate;

bool connection_lost = false;

state_t global;

void setup()
{
  SerialUSB.begin(115200);
  FastLED.addLeds<WS2811_PORTD, MaxRibbonsCount>(leds, MaxLedsPerRibbon);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.setMaxRefreshRate(50);

  for (size_t i=0 ; i<PRESETS_COUNT ; ++i)
  {
    beat_detectors[i].clock = osc_clocks + i;
  }

  FastLED.delay(1000);
}

void update_clocks()
{
  master_clock.setPeriod(1 + ((60lu * 1000lu * 100lu) / ((global.master.bpm + 1) * 2)));
  strobe_clock.setPeriod(max8(2, scale8(10, 255 - (global.master.strobe_speed << 1))));

  for (size_t i=0 ; i<PRESETS_COUNT ; ++i)
  {
    uint8_t clockmul = global.presets[i].speed_scale >> (7 - 3);
    uint32_t clockperiod = master_clock.period;
    if (clockmul < 9)
      clockperiod = clockperiod >> clockmul;
    else
      clockperiod = clockperiod << (9 - clockmul);
    if (96 <= global.presets[i].maskmod_osc)
      clockperiod = clockperiod >> 2;
    osc_clocks[i].setPeriod(clockperiod);
  }

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
    

    uint8_t feedback_per_group[3] = { 0 };

    // Firt reset the ribbon according to fade out
    for (uint8_t preset_index = 0 ; preset_index < 8 ; ++preset_index)
    {
      const state_t::preset_t& preset = global.presets[preset_index];
      
      if (!preset.feedback_enable)
        continue;
      else
      {
        const uint8_t preset_group = 0;//Global.ribbons[preset_index].group;
        if (preset.brightness < 8)
          continue;
        else
          feedback_per_group[preset_group] = max8(feedback_per_group[preset_group], preset.feedback_qty << 1);
      }
    }
    for (uint8_t ribbon = 0 ; ribbon < global.setup.ribbons_count ; ++ribbon)
    {
      uint8_t feedback = feedback_per_group[0];//Global.ribbons[ribbon].group];
      uint32_t ribbon_length = 30 * global.setup.ribbons_lengths[ribbon];
      CRGB* ribbon_ptr = leds + ribbon * MaxLedsPerRibbon;
      
      if (feedback == 0)
        fill_solid(ribbon_ptr, ribbon_length, CRGB::Black);
      else
        fadeToBlackBy(ribbon_ptr, ribbon_length, 255 - feedback);
    }

    /*
     * We apply each preset on all grouped ribbons
     */
    for (uint8_t preset_index = 0 ; preset_index < 8 ; ++preset_index)
    {
      const state_t::preset_t& preset = global.presets[preset_index];
      // Skip computation if brightness is 0
      if (preset.brightness == 0)
        continue;

      const uint8_t time = osc_clocks[preset_index].get8() + global.master.sync_correction;
        
      // The group which the preset is attached to
      const uint8_t preset_group = 0; //Global.ribbons[preset_index].group;
      //const objects::Group& group = Global.groups[preset_group];
      const uint8_t palette_index = preset.palette >> (7 - 3);
      const uint8_t palette_subindex = (preset.palette % 16) << 4;
      const auto& palette = global.palettes[palette_index];
      //const auto& paletteB = global.palettes[(palette_index +1) % 8];
      //const auto& palette = lerp_palette(paletteA, paletteB, palette_subindex);
      
      for (uint8_t ribbon_index = 0 ; ribbon_index < global.setup.ribbons_count ; ++ribbon_index)
      {
        //const objects::Ribbon& ribbon = Global.ribbons[ribbon_index];
        
        uint32_t ribbon_length = 30 * global.setup.ribbons_lengths[ribbon_index];
        CRGB* ribbon_ptr = leds + ribbon_index * MaxLedsPerRibbon;
        const size_t ribbon_byte_size = ribbon_length * sizeof(CRGB);

        // Break if ribbon is associated with another group
        //if (ribbon.group != preset_group)
        //  continue;

        OscillatorKind colormod_kind = map_to_oscillator_kind(preset.colormod_osc << 1);
        OscillatorKind maskmod_kind = map_to_oscillator_kind(preset.maskmod_osc << 1);
        uint8_t colormod_osc = eval_oscillator(colormod_kind, OscillatorKind::Noise == colormod_kind ? time << 2 : time);
        uint8_t maskmod_osc  = eval_oscillator(maskmod_kind, OscillatorKind::Noise == maskmod_kind ? time << 2 : time);
        
        if (OscillatorKind::Noise == colormod_kind || OscillatorKind::Noise == maskmod_kind)
        {
          if (beat_detectors[preset_index].trigger)
          {
            holded_values[preset_index][ribbon_index][0] = colormod_osc;
            holded_values[preset_index][ribbon_index][1] = maskmod_osc;
          }
          else
          {
            if (OscillatorKind::Noise == colormod_kind) colormod_osc = holded_values[preset_index][ribbon_index][0];
            if (OscillatorKind::Noise == maskmod_kind) maskmod_osc = holded_values[preset_index][ribbon_index][1];
          }
        }
  
        // Build the compo according to parameters
        const Composition compo{
          // Color modulation
          PaletteRangeController {
            // Center
            preset.colormod_enable ?
              scale8(
                colormod_osc, 
                preset.colormod_width << 1
                )
              : preset.colormod_osc << 1
            ,
            // Modulation depth
            preset.colormod_move ?
              scale8(
                colormod_osc,
                preset.colormod_width << 1
                )
              : preset.colormod_width << 1
          },
          Mask {
            // center : running point or static
            preset.maskmod_move ? 
              maskmod_osc
              : 0,
            // width
            preset.maskmod_move ?
              preset.maskmod_width
              : scale8(eval_oscillator(map_to_oscillator_kind(preset.maskmod_osc << 1), time), preset.maskmod_width << 1),
            // wraparound or saturate
            preset.maskmod_move,
            preset.maskmod_enable
          },
          // Ribbons splitting
          Slicer {
            // nslices from 1 to two slices per module
            1 + scale8(global.setup.ribbons_lengths[ribbon_index] * 4, preset.slicer_nslices << 1),
            // uneven factor
            preset.slicer_mergeribbon ? 255 : min8(uint8_t(preset.slicer_nuneven << 1), 253),
            // flip even slices
            preset.slicer_useflip,
            // use uneven slices
            preset.slicer_useuneven
          },
        };
          
        if (preset.strobe_enable && !strobe_clock.coarse_value)
          continue;
        
        for (uint32_t i = 0; i < ribbon_length; ++i) {
          CRGB c = compo.eval(palette, time, i, ribbon_length);
          CRGB o = ribbon_ptr[i];
          uint8_t bright = dim8_video(preset.brightness << 1);
          nscale8_video(&c, 1, bright);
         
          ribbon_ptr[i] = CRGB(max8(c.r, o.r), max8(c.g, o.g), max8(c.b, o.b));
        }
        
      } // for ribbon
    } // for preset
    if (global.master.blur_enable)
    {
      for (uint8_t ribbon_index = 0; ribbon_index < global.setup.ribbons_count; ++ribbon_index)
        for (uint8_t pass = 0 ; pass < (global.master.blur_qty >> (7 - 3)) ; ++pass)
          blur1d(leds + ribbon_index * MaxLedsPerRibbon, global.setup.ribbons_lengths[ribbon_index], global.master.blur_qty << 1);
    }
    unsigned long compute_end = millis();

    // Draw frame
    unsigned long draw_begin = millis();
    if (connection_lost)
      for (size_t i = 0 ; i < 30 ; ++i)
        leds[i] = master_clock.get8() < 0x7F ? CRGB::Red : CRGB::Black;
    FastLED.show(global.master.brightness);
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
      coarse_framerate = (uint8_t)fps;
      
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
      SerialUSB.println(global.master.bpm);
      SerialUSB.print(" : Strobe Period : ");
      SerialUSB.print(strobe_clock.period);
      SerialUSB.print(" : Coarse FPS : ");
      SerialUSB.print(coarse_framerate);
      SerialUSB.print(" : Ctrl : ");
      SerialUSB.println(global.master.strobe_speed);

      fps_accumulator = 0;
      frame_cptr = 0;
    }
}

int read_from_controller() {

  static uint8_t serial_buffer[512];
  static size_t serial_index = 0;
  static size_t data_address = 0;
  static size_t data_size = 0;

  unsigned long timeout_timestamp = millis();
  while (SerialUSB.available() <= 0)
  {
    if (SerialUSB_MESSAGE_TIMEOUT < millis() - timeout_timestamp)
    {
      return 0;
    }
    delayMicroseconds(100);
  }
  while (0 < SerialUSB.available())
  {
    int in = SerialUSB.read();
    if (in < 0)
      continue;
      
    serial_buffer[serial_index++] = in;
    
    if (serial_index == 2)
    {
      data_address = ((uint16_t)serial_buffer[0]) << 8 | serial_buffer[1];
    }
    else if (serial_index == 3)
    {
      data_size = serial_buffer[2];
    }
    else if (serial_index == 3 + data_size)
    {
      memcpy(((uint8_t*)&global) + data_address, serial_buffer + 3, data_size);
      SerialUSB.print("Written "); SerialUSB.print(data_size); SerialUSB.print(" bytes at addr ");
      SerialUSB.print(data_address); SerialUSB.println();
      serial_index = 0;
    }
  }
  return 0;
}
