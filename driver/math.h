#pragma once

/// Maps [0, 255] to [0, 255], just like x^4 maps [0, 1] to [0, 1]
inline uint8_t fourth_power(uint8_t x)
{
    auto x32 = static_cast<uint32_t>(x);
    auto x32_sq = x32 * x32; // Temporary value to reduce the number of multiplications. Instead of x * x * x * x we do x_sq = x * x ; x_4th = x_sq * x_sq (2 multiplications instead of 3) 
    return static_cast<uint8_t>((x32_sq * x32_sq) >> 24);
}