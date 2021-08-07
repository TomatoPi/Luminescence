#pragma once

#include "EffectInput.h"

using FinalEffect = float (*)(EffectInput);
using Effect      = float (*)(EffectInput, int);

#define FINAL_EFFECT(fn_name) float(fn_name)(EffectInput in)
#define EFFECT(fn_name)       float(fn_name)(EffectInput in, int idx)