#pragma once

#include "GeneratorInput.h"

using Generator     = float (*)(GeneratorInput);
using MetaGenerator = float (*)(GeneratorInput, int);

#define NUM_LAYERS 4

class Generators {
public:
    static void Init();

    static float eval(GeneratorInput in, int next_layer_idx = NUM_LAYERS - 1)
    {
        return next_layer_idx >= 0 ? layers[next_layer_idx](in, next_layer_idx - 1) : base(in);
    }

    static Generator     base;
    static MetaGenerator layers[NUM_LAYERS];
};

float BScrollingGradient(GeneratorInput in);

float MIdentity(GeneratorInput in, int idx);

float MBlink(GeneratorInput in, int idx);