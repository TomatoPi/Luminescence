#define OPTOPOULPE_MAXIMATOR
// #define OPTOPOULPE_SATELITE
// #define OPTOPOULPE_MIRRORS

#include <Arduino.h>
#include "color_palette.h"
#include "clock.h"
#include <FastLED.h>
#include "math.h"
#include "Composition.h"
#include "state.h"
#include <Ethernet.h>

// #define SERIAL SerialUSB
#define SERIAL Serial

// WS2811_PORTD: 25,26,27,28,14,15,29,11

// Nano : Leds data : D2 : Index select : D3
//  Set D3 to ground for solo driver 0, leave it floating for driver 1

/*
 * TODO :
 * 
 * Color modulation is too harsh, need rework
 * Continuous blending is ugly, need rework palettes and some controls
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
#ifdef OPTOPOULPE_MAXIMATOR

  #define DEVICE_UID 0xA0
  #define DEVICE_RIBBONS_COUNT 8
  #define DEVICE_RIBBON_SIZE   (8 * 30) /* 10 modules */

#endif

#ifdef OPTOPOULPE_SATELITE

  #define SATELITE_INDEX 0

  #define DEVICE_UID (0xB0 + SATELITE_INDEX)
  #define DEVICE_RIBBONS_COUNT 1
  #define DEVICE_RIBBON_SIZE   15 /* 5 modules */

#endif

#ifdef OPTOPOULPE_MIRRORS

  #define DEVICE_UID 0xC0
  #define DEVICE_RIBBONS_COUNT 1
  #define DEVICE_RIBBON_SIZE   32 /* Random magic number */

#endif

#define DEVICE_MAC_ADDRESS  0xDE, 0xAD, 0xEF, 0xBE, 0xED, (DEVICE_UID)
#define DEVICE_IPv4_ADDRESS 10, 43, 0, (DEVICE_UID)
#define DEVICE_PORT         0x1936

#define DEVICE_SERIAL_BAUDRATE 115200

#define MAX_LEDS_COUNT ((DEVICE_RIBBONS_COUNT) * (DEVICE_RIBBON_SIZE))

namespace ethernet {
  static byte            mac[] = { DEVICE_MAC_ADDRESS };
  static IPAddress       ip( DEVICE_IPv4_ADDRESS );
  static EthernetServer  server( DEVICE_PORT );
  static EthernetClient  client = EthernetClient();
}

#define PRINT(x) \
  Serial.print(x); \
  if (ethernet::client) \
    ethernet::client.print(x); \
  else \
    ethernet::server.print(x)

#define PRINTLN(x) \
  Serial.println(x); \
  if (ethernet::client) \
    ethernet::client.println(x); \
  else \
    ethernet::server.println(x)

#define ENDL() PRINTLN('\0')

namespace LOG {

  enum mode {
    Debug   = 0,
    Info    = 1,
    Warning = 2,
    Severe  = 3,
    Critical= 4,
  };

  struct internal {};

  static mode level = mode::Debug;

  bool print(LOG::mode Mode)
  {
    if (Mode < LOG::level)
      return false;
    
    PRINT(Ethernet.localIP());
    PRINT(":");
    PRINT(DEVICE_PORT);
    PRINT(" - ");
    return true;
  }
}

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

color_t leds[MAX_LEDS_COUNT];

Clock master_clock;
Clock osc_clocks[PRESETS_COUNT];
FallDetector beat_detectors[PRESETS_COUNT];
uint8_t holded_values[PRESETS_COUNT][MaxRibbonsCount][2];

FastClock strobe_clock;
uint8_t coarse_framerate;

state_t global;

void setup()
{
  SERIAL.begin(DEVICE_SERIAL_BAUDRATE);

  Ethernet.begin(ethernet::mac, ethernet::ip);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    // BAIL CRITICAL ERROR
  }

  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    delay(100);
    // BAIL SEVERE ERROR
  }
  
  ethernet::server.begin();
  if (print(LOG::Info))
  {
    PRINT("Opto-v2 : Server is ready at ");
    PRINT(Ethernet.localIP());
    ENDL();
  }

#ifdef OPTOPOULPE_MAXIMATOR
  FastLED.addLeds<WS2811_PORTD, DEVICE_RIBBONS_COUNT>(leds, DEVICE_RIBBON_SIZE);
#endif
#ifdef OPTOPOULPE_SATELITE
  FastLED.addLeds<WS2812B, 3, RGB>(leds, MaxLedsPerRibbon);
  FastLED.addLeds<WS2812B, 5, RGB>(leds + MaxLedsPerRibbon, MaxLedsPerRibbon);
#endif
#ifdef OPTOPOULPE_MIRRORS
  FastLED.addLeds<P9813, 3, 13, RGB>(leds, MaxLedsCount);
#endif

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
    const uint8_t ribbons_count = min(global.setup.ribbons_count, MaxRibbonsCount);

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
    for (uint8_t ribbon = 0 ; ribbon < ribbons_count ; ++ribbon)
    {
      uint8_t feedback = feedback_per_group[0];//Global.ribbons[ribbon].group];
      uint32_t ribbon_length = min(30 * global.setup.ribbons_lengths[ribbon], DEVICE_RIBBON_SIZE);
      CRGB* ribbon_ptr = leds + ribbon * DEVICE_RIBBON_SIZE;
      
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
      if (!preset.do_litmax && preset.brightness == 0)
        continue;
      
      if (preset.strobe_enable && !strobe_clock.coarse_value)
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

      for (uint8_t ribbon_index = 0 ; ribbon_index < ribbons_count ; ++ribbon_index)
      {
//        bool is_solodriver = ribbon_index == 7;
//        if (is_solodriver)
//        {
//          if (global.master.solo_enable)
//            if ((solodriver_index * 2 + ribbon_index) != )
//              continue;
//        }

        bool is_solo_ribbon = false; // ribbon_index == 7;
        
        uint32_t ribbon_leds_count = min(30 * global.setup.ribbons_lengths[ribbon_index], MaxLedsPerRibbon);
        uint8_t ribbon_modules_count = ribbon_leds_count / 30;
        CRGB* ribbon_ptr = leds + ribbon_index * MaxLedsPerRibbon;

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
            1 + scale8(ribbon_modules_count * 4, preset.slicer_nslices << 1),
            // uneven factor
            preset.slicer_mergeribbon ? 255 : min8(uint8_t(preset.slicer_nuneven << 1), 253),
            // flip even slices
            preset.slicer_useflip,
            // use uneven slices
            preset.slicer_useuneven
          },
        };
        
        uint8_t bright = preset.do_litmax ? 255 : dim8_video(preset.brightness << 1);
        if (!is_solo_ribbon && global.master.solo_enable && !preset.do_ignore_solo)
        {
          // a number between 0 and 4 excluded indicating which ribbon is soloing
          uint8_t soloindex = global.setup.soloribbons_location[global.master.solo_index];
          uint8_t soloside = soloindex / 2;
          uint8_t ribbonside = ribbon_index < (ribbons_count /2);

          bright = scale8_video(bright, dim8_video(ribbonside == soloside ? global.master.solo_weak_dim : global.master.solo_strong_dim));
        }
          
        for (uint32_t i = 0; i < ribbon_leds_count; ++i) {
          CRGB c = compo.eval(palette, time, i, ribbon_leds_count);
          CRGB o = ribbon_ptr[i];
          nscale8_video(&c, 1, bright);
         
          ribbon_ptr[i] = CRGB(max8(c.r, o.r), max8(c.g, o.g), max8(c.b, o.b));
        }
        
      } // for ribbon
    } // for preset
    unsigned long compute_end = millis();

    // Draw frame
    unsigned long draw_begin = millis();
    // When solowing, all ribbons on the other side than the soloing one are strongly dimmed
    //  All ribbons on the same side are weakly dimmed
    FastLED.show(global.master.do_kill_lights ? 0 : global.master.brightness);
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
      
      static uint64_t cptr = 0;
      if (LOG::print(LOG::Info))
      {
        PRINT((unsigned long int) cptr++);
        PRINT(" - ");
        PRINT("Avg FPS : ");
        PRINT(fps);
        PRINT(" : SerialUSB : ");
        PRINT(update_end - update_begin);
        PRINT(" : Compute : ");
        PRINT(compute_end - compute_begin);
        PRINT(" : Draw : ");
        PRINT(draw_end - draw_begin);
        PRINT(" : Drops : ");
        PRINT(drop_count);
        PRINT(" : Master Clock : ");
        PRINTLN(global.master.bpm);
        PRINT(" : Strobe Period : ");
        PRINT(strobe_clock.period);
        PRINT(" : Coarse FPS : ");
        PRINT(coarse_framerate);
        PRINT(" : Ctrl : ");
        PRINTLN(global.master.strobe_speed);
        ENDL();
      }

      fps_accumulator = 0;
      frame_cptr = 0;
    }
}

int read_from_controller() {

  static uint8_t serial_buffer[256];
  static size_t serial_index = 0;
  static size_t data_address = 0;
  static size_t data_size = 0;

  unsigned long timeout_timestamp = millis();

  if (ethernet::client && !ethernet::client.connected())
  {
    if (LOG::print(LOG::Info))
    {
      PRINT("Kill dead client ...");
      ENDL();
    }
    ethernet::client.stop();
  }

  if (!ethernet::client)
  {
    ethernet::client = ethernet::server.accept();
    if (!ethernet::client)
      return false;
    serial_index = 0;
    Serial.end();
    Serial.begin(DEVICE_SERIAL_BAUDRATE);
    if (LOG::print(LOG::Info))
    {
      PRINT("New client accepted");
      ENDL();
    }
  }

  while (ethernet::client && ethernet::client.connected() && 0 < ethernet::client.available())
  {
    int in = ethernet::client.read();
    if (in < 0)
      continue;
      
    serial_buffer[serial_index++] = in;
    
    if (serial_index == 1)
    {
      // FIX device not responding due to desynchronisation of programs
      if (serial_buffer[0] != 0xFF)
      {
        if (LOG::print(LOG::Info))
        {
          PRINT("Invalid packet received : Bad Header :");
          PRINT((int)serial_buffer[0]);
        }
        serial_index = 0;
        continue;
      }
    }
    else if (serial_index == 3)
    {
      data_address = ((uint16_t)serial_buffer[1]) << 8 | serial_buffer[2];
    }
    else if (serial_index == 4)
    {
      data_size = serial_buffer[3];
    }
    else if (serial_index == 4 + data_size)
    {
      // FIX random crashes occuring on bad transferts
      if (sizeof(state_t) < data_address + data_size)
      {
        if (LOG::print(LOG::Info))
            PRINT("Invalid packet received : Bad Addr");
        serial_index = 0;
        continue;
      }
      memcpy(((uint8_t*)&global) + data_address, serial_buffer + 4, data_size);

      if (LOG::print(LOG::Debug))
      {
        PRINT("Wrote ");
        PRINT(data_size);
        PRINT(" bytes at addr ");
        PRINT(data_address);
        ENDL();
      }
      serial_index = 0;
    }
  }
  return 0;
}
