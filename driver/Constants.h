#pragma once

#include <stdint.h>

#ifdef ARDUINO_SAM_DUE

static constexpr uint32_t MaxLedsPerRibbon = 30 * 8;
static constexpr uint32_t MaxRibbonsCount = 8;
static constexpr uint32_t MaxLedsCount = MaxRibbonsCount * MaxLedsPerRibbon;

#else // ifdef ARDUINO_SAM_DUE

static constexpr uint32_t MaxLedsPerRibbon = 30 * 1;
static constexpr uint32_t MaxRibbonsCount = 2;
static constexpr uint32_t MaxLedsCount = 1; //MaxRibbonsCount * MaxLedsPerRibbon;

#endif // ifdef ARDUINO_SAM_DUE
