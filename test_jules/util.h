#pragma once

#define NUM_LEDS 16

inline int clamp(int i) { return min(max(i, 0), 255); }

bool in_bounds(int i) { return i >= 0 && i < NUM_LEDS; }

float fract(float x)
{
    return x - floor(x);
}