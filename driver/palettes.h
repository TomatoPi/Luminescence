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

  static constexpr ColorPalette acid {
  // min  max  f*60 phase
    { 69, 255, 119, 120}, // RED
    { 47, 255, 179,  84}, // GREEN
    {111, 255, 120,  39}, // BLUE
  };

  static constexpr ColorPalette cocktail {
  // min  max  f*60 phase
    {  7, 255,  60,  28}, // RED
    {  5, 255,  60,   0}, // GREEN
    { 15, 255,  60,  18}, // BLUE
  };

  static constexpr ColorPalette many_colors {
  // min  max  f*60 phase
    { 86,  61, 120,  81}, // RED
    {140,   0, 240,   0}, // GREEN
    { 15, 248, 180,  18}, // BLUE
  };

  static constexpr ColorPalette bonbons {
  // min  max  f*60 phase
    { 61,  86, 120,  81}, // RED
    {  0, 140, 240,   0}, // GREEN
    { 15, 248, 180,  18}, // BLUE
  };

  static constexpr ColorPalette blue {
  // min  max  f*60 phase
    { 61,  86, 122,  81}, // RED
    {  0, 140, 240,   0}, // GREEN
    {248, 251, 182,  18}, // BLUE
  };

  static constexpr ColorPalette sunset {
  // min  max  f*60 phase
    {203, 255, 120, 117}, // RED
    { 53,  81,  60, 130}, // GREEN
    {  0, 163,  60,  49}, // BLUE
  };

  static constexpr ColorPalette carnival {
  // min  max  f*60 phase
    {  0, 201, 110, 134}, // RED
    {  0, 255, 179,   0}, // GREEN
    {  0, 242, 138,  60}, // BLUE
  };

  static constexpr ColorPalette no_blue {
  // min  max  f*60 phase
    {  0, 255, 110, 134}, // RED
    {  0, 255, 179,   0}, // GREEN
    {  0,   0, 138, 194}, // BLUE
  };

  static constexpr ColorPalette no_green {
  // min  max  f*60 phase
    {  0, 255, 110, 134}, // RED
    {  0,   0, 138, 194}, // GREEN
    {  0, 255, 179,   0}, // BLUE
  };

  static constexpr ColorPalette no_red {
  // min  max  f*60 phase
    {  0,   0, 138, 194}, // RED
    {  0, 255, 110, 134}, // GREEN
    {  0, 255, 179,   0}, // BLUE
  };

  static constexpr ColorPalette greenfield {
  // min  max  f*60 phase
    {  0, 255, 110, 134}, // RED
    {122, 255, 179, 130}, // GREEN
    {  0,  48, 138,  49}, // BLUE
  };

  const ColorPalette& Get(uint8_t idx)
  {
    switch (idx)
    {
      case 0:  return rainbow;
      case 1:  return deep_blue_and_bright_yellow;
      case 2:  return pastel;
      case 3:  return warm_beech;
      case 4:  return synthwave;
      case 5:  return acid;
      case 6:  return cocktail;
      case 7:  return many_colors;
      case 8:  return bonbons;
      case 9:  return blue;
      case 10: return sunset;
      case 11: return carnival;
      case 12: return no_blue;
      case 13: return no_green;
      case 14: return no_red;
      case 15: return greenfield;
      default: return warm_beech;
    }
  }

}
