#pragma once

#include <FastLED.h>
#include "OscillatorKind.h"

/**
 * @brief Describes a portion of a palette.
 * For example {0, 255} would be the whole palette and {0, 127} the first half
 * Note that {127, 126} is the whole palette, but starting in the middle (and still going in the same direction as {0, 255}).
 */
struct PaletteRange {
    uint8_t begin;
    uint8_t end;
};

struct PaletteRangeController {
    uint8_t center;
    uint8_t width;

    PaletteRange range() const {
        return {sub8(center, width),
                add8(center, width)};
    }
};

uint8_t position_in_palette(const PaletteRange& range, uint8_t position_in_palette_range)
{
    return map8(position_in_palette_range, range.begin, range.end);  
}
