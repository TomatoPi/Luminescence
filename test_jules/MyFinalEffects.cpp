#include "MyFinalEffects.h"
#include "ColorPalette.h"
#include "EffectsMixer.h"

namespace FinalEffects {

FINAL_EFFECT(ScrollingGradient)
{
    return palette_rainbow.eval(in.pos + in.time);
}

} // namespace FinalEffects