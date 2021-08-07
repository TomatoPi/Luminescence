#include <FastLED.h>
#define NUM_LEDS 16
#define DATA_PIN 2

CRGB leds[NUM_LEDS];

bool in_bounds(int i) {
  return i >= 0 && i < NUM_LEDS;
}

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
}

void loop() {
  gradient();
  // test();
}

void gradient() {
  static int frame = 0;
  for (int i = 0; i < NUM_LEDS; ++i) {
    float t = sin(frame*0.02 + i * 0.1) * 0.5 + 0.5;
    int r = t        * 255;
    int b = (1. - t) * 255;
    leds[i] = CRGB(r*0.3, 30*0.3, b*0.3);
  }
  FastLED.show(); delay(50);
  frame = (frame + 1);
}

int mirror(int idx) {
  return 
      idx < 0         ? - idx
    : idx >= NUM_LEDS ? 2 * NUM_LEDS - idx
    :                   idx;
}

int clamp(int i) {
  return min(max(i, 0), 255);
}

void ping_pong() {
  static int blue_idx = 0;
  static int blue_dir = 1;
  static int red_idx = NUM_LEDS - 6;
  static int red_dir = 2;

  memset(leds, 0, sizeof(leds));

  for (int i = 0; i < 16; ++i) {
    int idx = mirror(blue_idx - blue_dir * i);
    leds[idx].b = clamp(leds[idx].b + clamp(200 - i * 10));
  }
  for (int i = 0; i < 16; ++i) {
    int idx = mirror(red_idx - red_dir * i);
    leds[idx].r = clamp(leds[idx].r + clamp(200 - i * 10));
  }
  FastLED.show(); delay(30);

  blue_idx += blue_dir;
  if (!in_bounds(blue_idx)) {
    blue_dir *= - 1;
    blue_idx += blue_dir;
  }
  
  red_idx += red_dir;
  if (!in_bounds(red_idx)) {
    red_dir *= - 1;
    red_idx += red_dir;
  }
}

void test() {
  static int frame = 0;
  for (int i = 0; i < NUM_LEDS; ++i) {
    float t = sin(frame*0.02) * 0.5 + 0.5;
    int g = t * 16;
    leds[i] = CRGB(g, g, g);
  }
  FastLED.show(); delay(50);
  frame = (frame + 1);
}
