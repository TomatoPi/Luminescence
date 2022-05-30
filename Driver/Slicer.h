#pragma once

#include "stdint.h"
#include "math.h"

struct Slicer
{
    uint8_t slices_count;
    uint8_t uneven_slices_factor;
    bool flip_every_other_slice;
    bool use_uneven_slices;

    uint8_t map_ribbon_to_slice(uint32_t pos_in_ribbon, uint32_t ribbon_size) const {
        if (!use_uneven_slices) {
            const uint32_t slice_size = ribbon_size / slices_count;
            const uint8_t pos8 = map_32_to_8(pos_in_ribbon % slice_size, slice_size);
            return maybe_flip(pos8, pos_in_ribbon / slice_size);
        }
        else {
            const uint8_t p = map_32_to_8(pos_in_ribbon, ribbon_size);
            uint8_t idx = 0;
            uint8_t separator = uneven_slices_factor;
            uint8_t prev_separator = 255;
            while(p < separator && idx < slices_count) {
                idx++;
                prev_separator = separator;
                separator = scale8(separator, uneven_slices_factor);
            }
            return maybe_flip(map_32_to_8(p - separator, prev_separator - separator), idx);
        }
    }

private: 
    uint8_t maybe_flip(uint8_t x, uint8_t index) const {
        return (flip_every_other_slice && index % 2 == 0) ? 255 - x : x;
    }
};
