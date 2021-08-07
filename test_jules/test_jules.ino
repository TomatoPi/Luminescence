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
    // INIT
    EffectsMixer::final = &FinalEffects::ScrollingGradient;
    for (auto& layer : EffectsMixer::layers) {
        layer = &Effects::Identity;
    }
    //
    // EffectsMixer::layers[6] = &Effects::FreezeTime;
    EffectsMixer::layers[0] = &Effects::ReduceIntensity;
    EffectsMixer::layers[1] = &Effects::InvertTime;
    // EffectsMixer::layers[2] = &Effects::SplitRangeInTwo;
    // EffectsMixer::layers[3] = &Effects::PingPong;
    // EffectsMixer::layers[4] = &Effects::SinusoidalBlink;
    // EffectsMixer::layers[5] = &Effects::OffsetRangeInTime;
}

void loop()
{
    static EffectInput input;

    for (int i = 0; i < NUM_LEDS; ++i) {
        input.pos      = i / (float)(NUM_LEDS - 1);
        const auto col = EffectsMixer::eval(input);
        leds[i]        = CRGB{
            clamp(col.x * 255),
            clamp(col.y * 255),
            clamp(col.z * 255)};
    }

    FastLED.show();
    delay(10);
    input.time = fract(input.time + 0.0003f);
}