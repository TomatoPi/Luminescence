#include "FastLED.h"

template <typename T>
struct Generator
{
  const T& operator() () const { return val; }
  T val;
};

template <typename Begin, typename End>
struct Range
{
  template <typename Func>
  void operator() (const Func& func) const
  {
    for (unsigned int i=begin() ; i != end() ; ++i)
      func(i);
  }
  Begin& begin;
  End& end;
};

template <typename R, typename G, typename B>
struct ColorGenerator
{
  CRGB operator() () const { return CRGB(red(), green(), blue()); }
  R& red;
  G& green;
  B& blue;
};

template <typename ColorGenerator>
struct Colorizer
{
  void operator() (unsigned int index) const { leds[index] = generator(); }
  CRGB* leds;
  ColorGenerator& generator;
};

template <typename Range, typename Modifier>
struct Applicator
{
  void operator() () const { range(modifier); }
  Range& range;
  Modifier& modifier;
};

template <typename T, typename Evolution>
struct Evolver
{
  void operator() () { evolution(object); }
  T& object;
  Evolution evolution;
};

#define NUM_LEDS 14
CRGB leds[NUM_LEDS];

using IndexGenerator = Generator<unsigned int>;
IndexGenerator ribbon_start{0};
IndexGenerator ribbon_middle{NUM_LEDS / 2};
IndexGenerator ribbon_end{NUM_LEDS};

using VariableRange = Range<IndexGenerator,IndexGenerator>;
VariableRange range_0 {ribbon_start, ribbon_middle};
VariableRange range_1 {ribbon_middle, ribbon_end};

using ChannelGenerator = Generator<uint8_t>;
using VariableColor = ColorGenerator<ChannelGenerator, ChannelGenerator, ChannelGenerator>;

ChannelGenerator red_channel_0{128}, green_channel_0{32}, blue_channel_0{0};
VariableColor ribbon_color_0{red_channel_0, green_channel_0, blue_channel_0};

ChannelGenerator red_channel_1{0}, green_channel_1{32}, blue_channel_1{128};
VariableColor ribbon_color_1{red_channel_1, green_channel_1, blue_channel_1};

using RangeColorizer = Applicator<VariableRange, Colorizer<VariableColor>>;
Colorizer<VariableColor> colorizer_0{leds, ribbon_color_0};
Colorizer<VariableColor> colorizer_1{leds, ribbon_color_1};

RangeColorizer range_0_colorizer{range_0, colorizer_0};
RangeColorizer range_1_colorizer{range_1, colorizer_1};

void evaluate_ribbon()
{
  range_0_colorizer();
  range_1_colorizer();
}

void evolve_ribbon()
{
  red_channel_0.val += 1; // overflow wanted
  blue_channel_0.val -= 1;

  red_channel_1.val += 1; // overflow wanted
  blue_channel_1.val -= 1;
}

void setup() {
  delay(3000);
  FastLED.addLeds<NEOPIXEL, 2>(leds, NUM_LEDS);
  memset(leds, 0, sizeof(CRGB) * NUM_LEDS);
  FastLED.show();
}

void loop() {
  evaluate_ribbon();
  FastLED.show();
  evolve_ribbon();
  delay(10);
}
