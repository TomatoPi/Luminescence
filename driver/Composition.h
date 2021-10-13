#pragma once

#include "PaletteRangeController.h"
#include "color_palette.h"
#include "Constants.h"

struct Composition {
    PaletteRangeController palette_range_ctrl;

    CRGB eval(const ColorPalette& palette,
              uint8_t time,
              uint32_t position_in_strip) const
    {
        return palette.eval(
            position_in_palette(palette_range_ctrl.range(time),
                                map_32_to_8(position_in_strip, LedsCount))
        );
    }
};
