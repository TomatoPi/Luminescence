#include "MyFinalEffects.h"
#include "ColorPalette.h"
#include "EffectsMixer.h"

static vec3 clamp01(vec3 v)
{
    return vec3(
        min(max(v.x, 0.f), 1.f),
        min(max(v.y, 0.f), 1.f),
        min(max(v.z, 0.f), 1.f));
}

namespace FinalEffects {

FINAL_EFFECT(ScrollingGradient)
{
    return clamp01(palette_test6.eval(in.pos * 0.1 + in.time));
}

} // namespace FinalEffects