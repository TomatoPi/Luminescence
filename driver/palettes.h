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

  // Realy Nice :)
  static constexpr ColorPalette test {
  //  min  max  pulsation  phase
    { 0,   255,    255,      127  }, // RED
    { 0,   255,    255,     51  }, // GREEN
    { 0,   255,    0,     64  }  // BLUE
  };

  static constexpr ColorPalette warm_beech {
    { 0, 127, 127, 255 },
    { 25, 127, 127, 255 },
    { 51, 127, 127, 255 }
  };

  const ColorPalette& Get(uint8_t idx)
  {
    switch (idx)
    {
      case 0: return rainbow;
      case 1: return deep_blue_and_bright_yellow;
      case 2: return test;
      case 3:
      case 4:
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
