#include "MyEffects.h"
#include "Arduino.h"
#include "EffectsMixer.h"


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

EFFECT(InvertTime)
{
    in.time = 1.f - in.time;
    return APPLY_NEXT_EFFECT();
}

EFFECT(InvertSpace)
{
    in.pos = 1.f - in.pos;
    return APPLY_NEXT_EFFECT();
}

EFFECT(SplitRangeInTwo)
{
    in.pos = in.pos < 0.5f
                 ? 2.f * in.pos
                 : 1.f - 2.f * (in.pos - 0.5f);
    return APPLY_NEXT_EFFECT();
}

EFFECT(PingPong)
{
    // in.time = in.time < 0.5f
    //               ? 2.f * in.time
    //               : 1.f - 2.f * (in.time - 0.5f);
    in.time = sin(3.141 * in.time);
    return APPLY_NEXT_EFFECT();
}

} // namespace Effects