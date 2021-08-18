#pragma once

namespace Palettes {

static constexpr ColorPalette rainbow {
//  min  max  pulsation  phase
  { 0,   255,    255,      0  }, // RED
  { 0,   255,    255,     85  }, // GREEN
  { 0,   255,    255,    171  }  // BLUE
};

static constexpr ColorPalette deep_blue_and_bright_yellow {
//  min  max  pulsation  phase
  { 0,   255,    255,      0  }, // RED
  { 0,   255,    255,     25  }, // GREEN
  { 0,   255,    255,     51  }  // BLUE
};

}