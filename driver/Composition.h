#pragma once

#include "PaletteRangeController.h"
#include "Slicer.h"
#include "color_palette.h"
#include "Constants.h"

struct Composition {
    PaletteRangeController palette_range_ctrl;
    Slicer slicer;

    CRGB eval(const ColorPalette& palette,
              uint8_t time,
              uint32_t position_in_ribbon,
              uint32_t ribbon_size) const
    {
        return palette.eval(
            position_in_palette(palette_range_ctrl.range(time),
                                slicer.map_ribbon_to_slice(position_in_ribbon, ribbon_size))
        );
    }
};
