#pragma once

#include <stdint.h>

#ifdef OPTOPOULPE_MAXIMATOR
static constexpr uint32_t MaxLedsPerRibbon = 30 * 8;
static constexpr uint32_t MaxRibbonsCount = 8;
static constexpr uint32_t MaxLedsCount = MaxRibbonsCount * MaxLedsPerRibbon;
#endif

#ifdef OPTOPOULPE_SATELITE
static constexpr uint32_t MaxLedsPerRibbon = 30 * 1;
static constexpr uint32_t MaxRibbonsCount = 2;
static constexpr uint32_t MaxLedsCount = MaxRibbonsCount * MaxLedsPerRibbon;
#endif

#ifdef OPTOPOULPE_MIRRORS
static constexpr uint32_t MaxLedsPerRibbon = 30;
static constexpr uint32_t MaxRibbonsCount = 1;
static constexpr uint32_t MaxLedsCount = MaxRibbonsCount * MaxLedsPerRibbon;
#endif