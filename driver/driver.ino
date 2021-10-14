#include "color_palette.h"
#include "palettes.h"
#include "clock.h"
#include "range.h"
#include "common.h"
#include <FastLED.h>
#include "apply_modulation.h"
#include "math.h"
#include "Composition.h"
#include "params.h"
#include <array>

#define SerialUSB_MESSAGE_TIMEOUT 20
#define SerialUSB_SLEEP_TIMEOUT 0
#define FRAME_REFRESH_TIMEOUT 0

using color_t = CRGB;
using index_t = uint32_t;
using coef_t = float;

// Needed with Arduino IDE;
constexpr const uint8_t SerialPacket::Header[3];

color_t leds[MaxLedsCount];

FastClock strobe_clock;
Clock osc_clocks[4];
Clock& master_clock = osc_clocks[3];

bool connection_lost = false;

uint8_t coarse_framerate;

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
  FastLED.addLeds<WS2811_PORTD, MaxRibbonsCount>(leds, MaxLedsPerRibbon);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  FastLED.setMaxRefreshRate(50);

  memset(leds, 30, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();
  FastLED.delay(100);
  memset(leds, 0, sizeof(CRGB) * MaxLedsCount);
  FastLED.show();

  Global.setup = {
    1,
    { 14 }
  };

  SerialUSB.write(STOP_BYTE);
  FastLED.delay(1000);
}

void update_clocks()
{
  master_clock.setPeriod(1 + ((60lu * 1000lu * 100lu) / ((Global.master.bpm + 1))) << 2);
  strobe_clock.setPeriod(max8(2, scale8(coarse_framerate, 255 - (Global.master.strobe_speed << 1))));
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
    
    const uint8_t time = master_clock.get8();// + Global.master.sync_correction;

    uint8_t feedback_per_group[3] = { 0 };

    // Firt reset the ribbon according to fade out
    for (uint8_t preset_index = 0 ; preset_index < 8 ; ++preset_index)
    {
      const objects::Preset& preset = Global.presets[preset_index];
      
      const modulator_control tracer_ctrl = {
        preset.encoders[3],
        preset.encoders[7],
        !!(preset.switches & 0x08),
        !!(preset.pads_states & 0x08)
      };

      if (!tracer_ctrl.enable)
        continue;
      else
      {
        const uint8_t preset_group = Global.ribbons[preset_index].group;
        uint8_t feedback = scale8_video(tracer_ctrl.pot0 << 1, preset.brightness << 1);
        feedback_per_group[preset_group] = max8(feedback_per_group[preset_group], feedback);
      }
    }
    for (uint8_t ribbon = 0 ; ribbon < Global.setup.ribbons_count ; ++ribbon)
    {
      uint8_t feedback = feedback_per_group[Global.ribbons[ribbon].group];
      uint32_t ribbon_length = 30 * Global.setup.ribbons_lengths[ribbon];
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
      const objects::Preset& preset = Global.presets[preset_index];
      // Skip computation if brightness is 0
      if (preset.brightness == 0)
        continue;
        
      // The group which the preset is attached to
      const uint8_t preset_group = Global.ribbons[preset_index].group;
      //const objects::Group& group = Global.groups[preset_group];
      const auto& palette = Palettes::Get(Global.master.encoders[preset_index] >> (7 - 4));
      
      for (uint8_t ribbon_index = 0 ; ribbon_index < Global.setup.ribbons_count ; ++ribbon_index)
      {
        const objects::Ribbon& ribbon = Global.ribbons[ribbon_index];
        
        uint32_t ribbon_length = 30 * Global.setup.ribbons_lengths[ribbon_index];
        CRGB* ribbon_ptr = leds + ribbon_index * MaxLedsPerRibbon;
        const size_t ribbon_byte_size = ribbon_length * sizeof(CRGB);

        // Break if ribbon is associated with another group
        if (ribbon.group != preset_group)
          continue;
        
        // Convert parameters
        const modulator_control colormod_ctrl = {
          preset.encoders[0],
          preset.encoders[4],
          !!(preset.switches & 0x01),
          !!(preset.pads_states & 0x01)
        };
        const modulator_control maskmod_ctrl = {
          preset.encoders[1],
          preset.encoders[5],
          !!(preset.switches & 0x02),
          !!(preset.pads_states & 0x02)
        };
        const modulator_control slicer_ctrl = {
          preset.encoders[2],
          preset.encoders[6],
          !!(preset.switches & 0x04),
          !!(preset.pads_states & 0x04)
        };
        const bool strobe_ctrl = !!(preset.pads_states & 0x10);
  
        // Build the compo according to parameters
        const Composition compo{
          // Color modulation
          PaletteRangeController {
            // Center
            colormod_ctrl.enable ?
              scale8(
                eval_oscillator(map_to_oscillator_kind(colormod_ctrl.pot0 << 1), time), 
                colormod_ctrl.pot1 << 1
                )
              : colormod_ctrl.pot0 << 1
            ,
            // Modulation depth
            colormod_ctrl.toggle ?
              scale8(
                eval_oscillator(map_to_oscillator_kind(colormod_ctrl.pot0 << 1), time), 
                colormod_ctrl.pot1 << 1
                )
              : colormod_ctrl.pot1 << 1
          },
          Mask {
            // center : running point or static
            maskmod_ctrl.toggle ? 
              eval_oscillator(map_to_oscillator_kind(maskmod_ctrl.pot0 << 1), time)
              : 0,
            // width
            maskmod_ctrl.toggle ?
              maskmod_ctrl.pot1
              : scale8(eval_oscillator(map_to_oscillator_kind(maskmod_ctrl.pot0 << 1), time), maskmod_ctrl.pot1 << 1),
            // wraparound or saturate
            maskmod_ctrl.toggle,
            maskmod_ctrl.enable
          },
          // Ribbons splitting
          Slicer {
            // nslices from 1 to 16
            1 + scale8(Global.setup.ribbons_lengths[ribbon_index], slicer_ctrl.pot0 << 1),
            // uneven factor
            uint8_t(slicer_ctrl.pot1 << 1),
            // flip even slices
            slicer_ctrl.toggle,
            // use uneven slices
            slicer_ctrl.enable
          },
        };
          
        if (strobe_ctrl && !strobe_clock.coarse_value)
          continue;
        
        for (uint32_t i = 0; i < ribbon_length; ++i) {
          CRGB color = compo.eval(palette, time, i, ribbon_length);
          uint8_t bright = qadd8(color.r, qadd8(color.g, color.b));
          ribbon_ptr[i] = nblend(ribbon_ptr[i], color, scale8_video(bright, preset.brightness << 1));
        }
        
      } // for ribbon
    } // for preset
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
      SerialUSB.println(Global.master.bpm);
      SerialUSB.print(" : Strobe Period : ");
      SerialUSB.print(strobe_clock.period);
      SerialUSB.print(" : Coarse FPS : ");
      SerialUSB.print(coarse_framerate);
      SerialUSB.print(" : Ctrl : ");
      SerialUSB.println(Global.master.strobe_speed);
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
//        SerialUSB.print(millis());
//        SerialUSB.println(": full blob ok");
//        SerialUSB.write(STOP_BYTE);

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
            //Global.setup = result.read<objects::Setup>();
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
