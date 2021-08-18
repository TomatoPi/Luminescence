#include "apply_modulation.h"
#include "common.h"
#include <FastLED.h>

uint8_t apply_modulation(
  const objects::Modulation& modulation,
  uint8_t value,
  uint8_t time,
  uint8_t space) 
{
  using namespace objects;
  switch (modulation.kind)
  {
    case flags::ModulationKind::None:
    {
      return value;
    }
    case flags::ModulationKind::Sin:
    {
      return value + map8(sin8(modulation.istimemod ? time : space), modulation.min, modulation.max);
    }
    default:
    {
      return value;
    }
  }
}