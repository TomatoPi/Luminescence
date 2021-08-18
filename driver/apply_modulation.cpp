#include "apply_modulation.h"
#include "common.h"
#include <FastLED.h>

static uint8_t apply_waveform(uint8_t input, objects::flags::ModulationKind modulation_kind, uint8_t nb_of_periods) 
{
  using namespace objects;
  input = input * nb_of_periods;
  switch (modulation_kind)
  {
    case flags::ModulationKind::None:
    {
      return 0;
    }
    case flags::ModulationKind::Sin:
    {
      return sin8(input);
    }
    case flags::ModulationKind::SawTooth:
    {
      return input;
    }
    case flags::ModulationKind::Square:
    {
      return input < 127 ? 0 : 127;
    }
    case flags::ModulationKind::Triangle:
    {
      return input < 127 ? 2 * input : 509 - 2 * input;
    }
    default:
    {
      return 0;
    }
  }
}

uint8_t apply_modulation(
  const objects::Modulation& modulation,
  uint8_t value,
  uint8_t time,
  uint8_t space) 
{
  uint8_t input = modulation.istimemod ? time : space;
  uint8_t modulated_input = apply_waveform(input, modulation.kind, modulation.subdivide);
  return value + map8(modulated_input, modulation.min, modulation.max);
}