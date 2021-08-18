#pragma once

#include <stdint.h>
namespace objects {
    struct Modulation;
}

/**
 * @brief Applies the given modulation to modify the current value based on time and space
 * 
 * @param value The current "value"
 * @param time 
 * @param space 
 * @return uint8_t The new "value"
 */
uint8_t apply_modulation(
    const objects::Modulation& modulation,
    uint8_t value,
    uint8_t time,
    uint8_t space);