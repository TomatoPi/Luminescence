#pragma once

#include <stdint.h>

static constexpr uint32_t MaxLedsPerRibbon = 30 * 16;
static constexpr uint32_t MaxRibbonsCount = 8;
static constexpr uint32_t MaxLedsCount = MaxRibbonsCount * MaxLedsPerRibbon;
