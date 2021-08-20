#pragma once

/**
 * 
 * Palettes can be easily generated using this tool :
 * https://github.com/JulesFouchy/ColorPaletteGenerator
 * 
 */

namespace Palettes {

  static constexpr ColorPalette rainbow {
  // min  max  f*60 phase
    {  0, 255,  60,   0}, // RED
    {  0, 255,  60,  85}, // GREEN
    {  0, 255,  60, 171}, // BLUE
  };

  static constexpr ColorPalette deep_blue_and_bright_yellow {
  // min  max  f*60 phase
    {  0, 255,  60,  25}, // GREEN
    {  0, 255,  60,   0}, // RED
    {  0, 255,  60,  51}, // BLUE
  };

  // Realy Nice :)
  static constexpr ColorPalette pastel {
  // min  max  f*60 phase
    {  0, 255,  60, 127}, // RED
    {  0, 255,  60,  51}, // GREEN
    {  0, 255,   0,  64}, // BLUE
  };

  static constexpr ColorPalette warm_beech {
  // min  max  f*60 phase
    {  0, 127,  30, 255 }, // RED
    { 25, 127,  30, 255 }, // GREEN
    { 51, 127,  30, 255 }, // BLUE
  };

  static constexpr ColorPalette synthwave {
  // min  max  f*60 phase
    { 69, 255, 120, 120}, // RED
    {  0,  56,   0,  84}, // GREEN
    {111, 255,   0,  39}, // BLUE
  };

  const ColorPalette& Get(uint8_t idx)
  {
    switch (idx)
    {
      case 0: return rainbow;
      case 1: return deep_blue_and_bright_yellow;
      case 2: return pastel;
      case 3: return warm_beech;
      case 4: return synthwave;
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
      default: return warm_beech;
    }
  }

}
