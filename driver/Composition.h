#pragma once

#include <FastLED.h>
#include "color_palette.h"

template<typename Uint>
struct Range {
    Uint begin;
    Uint end;
};

using PixelRange = Range<uint32_t>;
using PaletteRange = Range<uint8_t>;

struct PositionInPalette {
    uint8_t center;
    uint8_t width;

    PaletteRange range() const {
        return {center - width,
                center + width};
    }
};

CRGB eval_color(const ColorPalette& palette,
                const PaletteRange& palette_range,
                uint8_t position_in_palette_range)
{
    const auto pos_in_palette = map8(position_in_palette_range, palette_range.begin, palette_range.end);  
    return palette.eval(pos_in_palette);
}

enum class Oscillator {
    Sine,
    Saw,
};

struct PositionInPaletteController {
    Oscillator oscillator;

    uint32_t center(uint8_t time) const {
        switch (oscillator)
        {
        case Oscillator::Sine:
            return sin8(time);
        
        default:
            return 0;
        }
    } 
};

struct Composition {

};