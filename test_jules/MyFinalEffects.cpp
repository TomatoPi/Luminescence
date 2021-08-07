#include "MyFinalEffects.h"
#include "EffectsMixer.h"

namespace FinalEffects {

FINAL_EFFECT(ScrollingGradient)
{
    return in.pos + in.time;
}

} // namespace FinalEffects