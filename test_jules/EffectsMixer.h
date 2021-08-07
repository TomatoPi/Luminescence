#pragma once

#include "Effect.h"

#define NUM_LAYERS 4

#define APPLY_NEXT_EFFECT() EffectsMixer::eval(in, idx)

struct EffectsMixer {
    static vec3 eval(EffectInput in, int next_layer_idx = NUM_LAYERS - 1)
    {
        return next_layer_idx >= 0 ? layers[next_layer_idx](in, next_layer_idx - 1) : final(in);
    }

    static FinalEffect final;
    static Effect      layers[NUM_LAYERS];
};
