#pragma once

#include "PaletteRangeController.h"
#include "Slicer.h"
#include "color_palette.h"
#include "Constants.h"
#include "Mask.h"

struct Composition {
    PaletteRangeController palette_range_ctrl;
    Mask mask;
    Slicer slicer;

    CRGB eval(const ColorPalette& palette,
              uint8_t time,
              uint32_t position_in_ribbon,
              uint32_t ribbon_size) const
    {
        const uint8_t rel_pos = slicer.map_ribbon_to_slice(position_in_ribbon, ribbon_size);
        CRGB color = ::eval(palette,position_in_palette(palette_range_ctrl.range(), rel_pos));
        uint8_t dim = mask.should_hide(rel_pos);
        return CRGB(scale8(color.r, dim), scale8(color.g, dim), scale8(color.b, dim));
    }
};
