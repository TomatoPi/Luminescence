#pragma once

#include "EffectInput.h"
#include "vec3.h"

using FinalEffect = vec3 (*)(EffectInput);
using Effect      = vec3 (*)(EffectInput, int);

// clang-format off
#define FINAL_EFFECT(fn_name) vec3(fn_name)(EffectInput in)
#define EFFECT(fn_name)       vec3(fn_name)(EffectInput in, int idx)
// clang-format on