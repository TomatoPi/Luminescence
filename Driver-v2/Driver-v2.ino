/// @file Driver-v2.ino : Arduino sketch running on Optopoulpe's led controllers

#define OPTOPOULPE_MAXIMATOR
// #define OPTOPOULPE_SATELITE
// #define OPTOPOULPE_MIRRORS

/* *** LIBRARY INCLUDES *** */

#include <Arduino.h>
#include <FastLED.h>
#include <Ethernet.h>

/* *** SKETCH INCLUDES *** */

#include "platforms/platforms.h"

/* *** PLATFORM DEFINES *** */

#ifdef OPTOPOULPE_MAXIMATOR

  #define DEVICE_UID 0xA0
  #define DEVICE_RIBBONS_COUNT 8
  #define DEVICE_RIBBON_SIZE   (1 * 5) /* 10 modules */

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
#define DEVICE_IPv4_ADDRESS 192, 168, 0, (DEVICE_UID)
#define DEVICE_PORT         0x1936

#define DEVICE_SERIAL_BAUDRATE 115200

#define PRINT(x) \
  Serial.print(x); \
  opto::ethernet::server.print(x)

#define PRINTLN(x) \
  Serial.println(x); \
  opto::ethernet::server.println(x)

#define CPRINT(x) \
  Serial.print(x); \
  opto::ethernet::server.print(x)

#define CENDL() \
  Serial.println(""); \
  client.println("")

// #define MPRINT(Mode, xx...) \
//   if (log::level <= Mode) { \
//     PRINT(Ethernet.localIP()); \
//     PRINT(" - "); \
//     PRINT(xx)... \
//     PRINTLN(""); \
//   }

/* *** GLOBALS *** */

namespace opto {

  namespace ethernet {
    static byte            mac[] = { DEVICE_MAC_ADDRESS };
    static IPAddress       ip( DEVICE_IPv4_ADDRESS );
    static EthernetServer  server( DEVICE_PORT );
    static EthernetClient  client;
  }

  namespace leds {
    
    static constexpr uint32_t MaxRibbonsCount = DEVICE_RIBBONS_COUNT;
    static constexpr uint32_t MaxLedsPerRibbon = DEVICE_RIBBON_SIZE;
    static constexpr uint32_t MaxLedsCount = MaxRibbonsCount * MaxLedsPerRibbon;

    static CRGB leds[MaxLedsCount];
  }

  namespace log {

    enum mode {
      Debug   = 0,
      Info    = 1,
      Warning = 2,
      Severe  = 3,
      Critical= 4,
    };

    struct internal {};

    static mode level = mode::Debug;
  }

  /* *** Log utilities *** */

  
  bool print(log::mode Mode)
  {
    if (Mode < log::level)
      return false;
    
    PRINT(Ethernet.localIP());
    PRINT(" - ");
    return true;
  }

  void endl()
  { PRINTLN(""); }

  // template <log::mode Mode, typename T, typename ... Args>
  // void print<Mode, log::internal, T, Args...>(T t, Args ...args)
  // { PRINT(t); print<Mode, log::internal,Args...>(args...); }

  // template <log::mode Mode, typename T>
  // void print<Mode, log::internal, T>(T t)
  // { PRINT(t); }

  /* *** Ethernet Receive Callback *** */

  bool read_from_ethernet () {

    using namespace ethernet;

    union serial_buffer {
      struct {
        char     header[4];
        uint16_t address;
        uint16_t size;
      };
      uint8_t raw[8] = {0};
    };
    static_assert(sizeof(serial_buffer) == 8, "");

    static serial_buffer _buffer;
    static size_t _index = 0;

    unsigned long timeout_timestamp = millis();

    if (client && !client.connected())
    {
      if (print(log::Info))
      {
        CPRINT("Kill dead client ...");
        CENDL();
      }
      client.stop();
    }

    if (!client)
    {
      client = ethernet::server.accept();
      if (!client)
        return false;
      _index = 0;
      if (print(log::Info))
      {
        CPRINT("New client accepted");
        CENDL();
      }
    }

    while (client && client.connected() && 0 < client.available())
    {
      int in = client.read();
      if (in < 0)
        continue;

      if (print(log::Debug))
      {
        CPRINT("Index : ");
        CPRINT(_index);
        CPRINT(" RCV : ");
        CPRINT(in);
        CENDL();
      }

      if (_index < sizeof(serial_buffer))
      {
        _buffer.raw[_index] = (uint8_t)in;
        if (_index < 4) {
          static const char validation[4] = {'O', 'p', 't', 'o'};
          if (_buffer.raw[_index] != validation[_index])
          {
            if (print(log::Debug))
            {
              CPRINT("Bad packet Header : '");
              CPRINT(_buffer.header);
              CPRINT("'");
              CENDL();
            }
            _index = 0;
            continue;
          }
        }
        _index += 1;
        continue;
      }

      const size_t data_index = _index - sizeof(serial_buffer);
      const size_t led_index = _buffer.address + (data_index / 3);

      if (print(log::Debug))
      {
        CPRINT("    ADDRESS=");
        CPRINT(_buffer.address);
        CPRINT("    SIZE=");
        CPRINT(_buffer.size);
        CENDL();
      }

      if (data_index < _buffer.size && led_index < leds::MaxLedsCount)
      {
        leds::leds[led_index].raw[data_index % 3] = (uint8_t)in;

        if (print(log::Debug))
        {
          CPRINT("    IDX=");
          CPRINT(led_index);
          CPRINT("    R=");
          CPRINT(leds::leds[led_index].r);
          CPRINT("    G=");
          CPRINT(leds::leds[led_index].g);
          CPRINT("    B=");
          CPRINT(leds::leds[led_index].b);
          CENDL();
        }
        // Loop to zero on the last byte
        _index = (_index + 1) % (sizeof(serial_buffer) + _buffer.size);
        continue;
      }

      // Message end
      _index = 0;
    }
    return true;
  }

} /* namespace opto */

/* ****** * SETUP * ****** */
using namespace opto;

void setup()
{
  Serial.begin(DEVICE_SERIAL_BAUDRATE);

  Ethernet.begin(opto::ethernet::mac, opto::ethernet::ip);
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    // BAIL CRITICAL ERROR
  }

  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
    delay(100);
    // BAIL SEVERE ERROR
  }

  opto::ethernet::server.begin();
  if (print(opto::log::Info))
  {
    PRINT("Opto-v2 : Server is ready at ");
    PRINT(Ethernet.localIP());
    endl();
  }

  #ifdef OPTOPOULPE_MAXIMATOR
    FastLED.addLeds<WS2811_PORTD, MaxRibbonsCount>(opto::leds::leds, MaxLedsPerRibbon);
  #endif
  #ifdef OPTOPOULPE_SATELITE
    FastLED.addLeds<WS2812B, 13, RGB>(opto::leds::leds, MaxLedsCount);
    // FastLED.addLeds<WS2812B, 5, RGB>(opto::leds::leds + MaxLedsPerRibbon, MaxLedsPerRibbon);
  #endif
  #ifdef OPTOPOULPE_MIRRORS
    FastLED.addLeds<P9813, 3, 13, RGB>(opto::leds::leds, MaxLedsCount);
  #endif

  // FastLED.setMaxPowerInVoltsAndMilliamps(5, 10000);
  // FastLED.setMaxRefreshRate(50);
}

/* ****** * LOOP * ****** */

void loop()
{
  /* *** UPDATE *** */

  unsigned long update_begin = millis(); 

  for (size_t i=0; i<opto::leds::MaxLedsCount ; ++i)
    opto::leds::leds[i] = CRGB::Red;
  
  opto::read_from_ethernet();

  unsigned long update_end = millis();

  /* *** PRINT *** */

  unsigned long draw_begin = millis();
  FastLED.show();
  unsigned long draw_end = millis();

  /* *** LOG *** */

  static unsigned long fps_accumulator = 0;
  static unsigned long frame_cptr = 0;
  static unsigned long last_timestamp = 0;

  fps_accumulator += draw_end - last_timestamp;
  frame_cptr += 1;
  if (2000 < fps_accumulator)
  {
    float fps = float(frame_cptr) * 1000.f / fps_accumulator;
    
    static uint64_t cptr = 0;
    if (print(opto::log::Info))
    {
      PRINT((unsigned long) cptr++);
      PRINT(" - Avg FPS : ");
      PRINT(fps);
      PRINT(" : Ethernet : ");
      PRINT(update_end - update_begin);
      PRINT(" : Draw : ");
      PRINT(draw_end - draw_begin);
      endl();
    }

    fps_accumulator = 0;
    frame_cptr = 0;
  }
  last_timestamp = draw_end;
  FastLED.delay(1);
}