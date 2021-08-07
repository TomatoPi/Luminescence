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
    return FORWARD();
}

EFFECT(Blink)
{
    in.time = in.time < 0.5f ? 0.f : 0.5f;
    return FORWARD();
}

EFFECT(FreezeTime)
{
    in.time = 0.f;
    return FORWARD();
}

} // namespace Effects