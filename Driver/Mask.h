#pragma once

#include <stdint.h>

uint8_t min8(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}
uint8_t max8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

struct Mask {
    uint8_t center;
    uint8_t half_width;
    bool should_wrap;
    bool enable;
    
    uint8_t should_hide(uint8_t rel_pos) const {
      if (!enable)
        return 255;
      else
      {
        uint8_t m = min8(rel_pos, center);
        uint8_t M = max8(rel_pos, center);
        uint8_t dist_to_center = should_wrap ? min8(M - m, m + (255 - M)) : M - m;
        return scale8(dist_to_center, half_width);
      }
    }
};
