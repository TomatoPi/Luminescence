#pragma once

#include "stdint.h"
#include "math.h"

struct Slicer
{
    uint8_t slices_count;
    bool flip_every_other_slice;

    uint8_t map_ribbon_to_slice(uint32_t pos_in_ribbon, uint32_t ribbon_size) const {
        const uint32_t slice_size = ribbon_size / slices_count;
        const uint8_t pos8 = map_32_to_8(pos_in_ribbon % slice_size, slice_size);
        if (flip_every_other_slice) {
            return (pos_in_ribbon / slice_size) % 2 == 0 ? pos8 : 255 - pos8;
        }
        else {
            return pos8;
        }
    }
};
