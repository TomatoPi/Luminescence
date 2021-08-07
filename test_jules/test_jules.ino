#include <FastLED.h>
#include "ColorPalette.h"
#include "EffectsMixer.h"
#include "MyEffects.h"
#include "MyFinalEffects.h"
#include "util.h"

#define DATA_PIN 2

CRGB leds[NUM_LEDS];

void setup()
{
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    EffectsMixer::final = &FinalEffects::ScrollingGradient;
    for (auto& layer : EffectsMixer::layers) {
        layer = &Effects::Identity;
    }
    EffectsMixer::layers[0] = &Effects::FreezeTime;
}

void loop()
{
    static EffectInput input;

    for (int i = 0; i < NUM_LEDS; ++i) {
        input.pos = i / (float)(NUM_LEDS - 1);
        float t   = EffectsMixer::eval(input);
        leds[i]   = palette_rainbow.eval(t);
    }
    FastLED.show();
    delay(50);
    input.time = fract(input.time + 0.05f);
}