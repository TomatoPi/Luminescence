#include "Effects.h"
#include "EffectsMixer.h"

namespace FinalEffects {

FINAL_EFFECT(ScrollingGradient)
{
    return in.pos + in.time;
}

} // namespace FinalEffects

namespace Effects {

EFFECT(Identity)
{
    return APPLY_NEXT_EFFECT();
}

EFFECT(Blink)
{
    in.time = in.time < 0.5f ? 0.f : 0.5f;
    return APPLY_NEXT_EFFECT();
}

EFFECT(FreezeTime)
{
    in.time = 0.f;
    return APPLY_NEXT_EFFECT();
}

} // namespace Effects