#include <FastLED.h>
#include "ColorPalette.h"
#include "Comet.h"
#include "Effects.h"
#include "EffectsMixer.h"
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
    // gradient(palette_rainbow);
    // ping_pong();
    // test();
}

void gradient(ColorPalette color_palette)
{
    static int frame = 0;
    for (int i = 0; i < NUM_LEDS; ++i) {
        float t = sin(frame * 0.02 + i * 0.1) * 0.5 + 0.5;
        leds[i] = color_palette.eval(t);
    }
    FastLED.show();
    delay(50);
    frame = (frame + 1);
}

int mirror(int idx)
{
    return idx < 0 ? -idx : idx >= NUM_LEDS ? 2 * NUM_LEDS - idx
                                            : idx;
}

void ping_pong()
{
    static Comet comet_blue{0, 1};
    static Comet comet_red{7, 1, 2};

    memset(leds, 10, sizeof(leds));

    for (int i = 0; i < comet_blue.length; ++i) {
        int idx = comet_blue.head_idx - comet_blue.dir * i;
        if (in_bounds(idx)) {
            float t   = comet_blue.eval(idx);
            CRGB  col = palette_yellow_blue.eval(t);
            leds[idx] += col;
        }
    }

    for (int i = 0; i < comet_red.length; ++i) {
        int idx = comet_red.head_idx - comet_red.dir * i;
        if (in_bounds(idx)) {
            float t   = comet_blue.eval(idx);
            CRGB  col = palette_rainbow.eval(t);
            leds[idx] += col;
        }
    }
    FastLED.show();
    delay(100);

    comet_blue.update();
    comet_red.update();
}

void test()
{
    static int frame = 0;
    for (int i = 0; i < NUM_LEDS; ++i) {
        float t = sin(frame * 0.02) * 0.5 + 0.5;
        int   g = t * 16;
        leds[i] = CRGB(g, g, g);
    }
    FastLED.show();
    delay(50);
    frame = (frame + 1);
}
