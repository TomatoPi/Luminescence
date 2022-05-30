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
    Serial.begin(9600);

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
     EffectsMixer::layers[3] = &Effects::PingPong;
    // EffectsMixer::layers[4] = &Effects::SinusoidalBlink;
    // EffectsMixer::layers[5] = &Effects::OffsetRangeInTime;
}

void loop()
{
    static EffectInput input;

    static int cptr = 0;
    static unsigned long fpscptr = 0;
    unsigned long begin = micros();
//    for (int i = 0; i < NUM_LEDS; ++i) {
//        input.pos      = i / (float)(NUM_LEDS - 1);
//        const auto col = EffectsMixer::eval(input);
//        leds[i]        = CRGB{
//            clamp(col.x * 255),
//            clamp(col.y * 255),
//            clamp(col.z * 255)};
//    }
    static constexpr unsigned long N = 1200;
    for (unsigned long i=0 ; i<N ; ++i)
      leds[i & 0xFF] = palette_rainbow.eval((float)i / N);

    unsigned long end = micros();
    FastLED.show();

    fpscptr += end - begin;
    if (cptr == 0)
    {
      Serial.print("Temps par frame : ");
      Serial.println((unsigned long)(fpscptr / N));
      Serial.print("Temps par led : ");
      Serial.println((unsigned long)(fpscptr / (N * NUM_LEDS)));
      Serial.println();
      fpscptr = 0;
    }
    
    delay(10);
    input.time = fract(input.time + 0.0003f);
}
