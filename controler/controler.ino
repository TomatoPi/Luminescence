#include <DMXSerial.h>

#define DMX_SELECT_PIN 2

void setup()
{
  DMXSerial.init(DMXController);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DMX_SELECT_PIN, OUTPUT);
  digitalWrite(DMX_SELECT_PIN, HIGH);
}

void loop()
{
  static uint8_t r = 0;
  static uint8_t g = 64;
  static uint8_t b = 128;

  r++, g++, b++;

  DMXSerial.write(0, r);
  DMXSerial.write(0, g);
  DMXSerial.write(0, b);

  digitalWrite(LED_BUILTIN, (r % 20) < 10);

  delay(20);
}
