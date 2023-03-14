/// @file Driver-v2.ino : Arduino sketch running on Optopoulpe's led controllers

// #define OPTOPOULPE_MAXIMATOR
#define OPTOPOULPE_SATELITE
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
  #define DEVICE_RIBBON_SIZE   300 /* 10 modules */

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

  /* *** Ethernet Receive Callback *** */

  bool read_from_ethernet () {

    using namespace ethernet;

    union serial_buffer {
      struct {
        char     header[4] = {0};
        uint16_t address = 0;
        uint16_t size = 0;
      };
      uint8_t raw[8];
    };
    static_assert(sizeof(serial_buffer) == 8, "");

    static serial_buffer _buffer;
    static size_t _index = 0;

    unsigned long timeout_timestamp = millis();

    if (client && !client.connected())
    {
      Serial.println("Kill dead client ...");
      client.stop();
    }

    if (!client)
    {
      client = ethernet::server.available();
      if (!client)
        return false;
      _index = 0;
      Serial.println("New client accepted");
    }

    while (client && client.connected() && 0 < client.available())
    {
      int in = client.read();
      if (in < 0)
        continue;

      PRINT("Index : ");
      PRINT(_index);
      PRINT(" RCV : ");
      PRINTLN(in);

      if (_index < sizeof(serial_buffer))
      {
        _buffer.raw[_index] = (uint8_t)in;
        if (_index < 4) {
          static const char validation[4] = {'O', 'p', 't', 'o'};
          if (_buffer.raw[_index] != validation[_index])
          {
            PRINT("Bad Packet Header : '");
            PRINT(_buffer.header);
            PRINTLN("'");
            _index = 0;
            continue;
          }
        }
        _index += 1;
        continue;
      }

      const size_t data_index = _index - sizeof(serial_buffer);
      const size_t led_index = _buffer.address + (data_index / 3);
      PRINT("ADDRESS=");
      PRINT(_buffer.address);
      PRINT(" SIZE=");
      PRINTLN(_buffer.size);

      if (data_index < _buffer.size && led_index < leds::MaxLedsCount)
      {
        leds::leds[led_index].raw[data_index % 3] = (uint8_t)in; 
        _index += 1 % (sizeof(serial_buffer) + _buffer.size);
        continue;
      }

      PRINT("Wrote ");
      PRINT(_index);
      PRINTLN(" Bytes");
      
      // Message end
      _index = 0;
    }
    return true;
  }

} /* namespace opto */

/* ****** * SETUP * ****** */

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
  Serial.print("server is ready at ");
  Serial.println(Ethernet.localIP());

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
  
  // opto::read_from_ethernet();

  for (size_t i=0; i<opto::leds::MaxLedsCount ; ++i)
    opto::leds::leds[i] = CRGB::Red;

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
    PRINT(Ethernet.localIP());
    PRINT(" - ");
    PRINT((unsigned long int) cptr++);
    PRINT(" - ");
    PRINT("Avg FPS : ");
    PRINT(fps);
    PRINT(" : Ethernet : ");
    PRINT(update_end - update_begin);
    PRINT(" : Draw : ");
    PRINTLN(draw_end - draw_begin);

    fps_accumulator = 0;
    frame_cptr = 0;
  }
  last_timestamp = draw_end;
  FastLED.delay(1);
}