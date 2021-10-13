#pragma once

#include <stdint.h>

uint8_t min(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}
uint8_t max(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

struct Mask {
    uint8_t center;
    uint8_t width;

    bool should_hide(uint8_t rel_pos) const {
        uint8_t m = min(rel_pos, center);
        uint8_t M = max(rel_pos, center);
        uint8_t dist_to_center = min(M - m, m + (255 - M));
        return dist_to_center > width / 2;        
    }
};