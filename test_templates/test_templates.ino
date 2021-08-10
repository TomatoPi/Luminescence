#include "FastLED.h"
#include "color_palette.h"
#include "fixed_point.h"
#include "midi.h"
#include "array.h"

using coef_t = float;

struct RenderDatas {
  unsigned int pixel_index = 0;
  CRGB old_color = CRGB::Black;

  unsigned int ribbon_length = 0;
  unsigned int ribbon_index = 0;
  unsigned int ribbon_count = 0;

  unsigned int segment_index = 0;
  unsigned int segment_count = 0;
};

CRGB render_color_wheel(const RenderDatas& datas, const void* _state)
{
  // Convert pixel index to position on a circle
  // based on number of leds per metter, pixel index for radial distance
  // ribbon index and ribbon count for angular position

  // Render a nice color wheel
  float h = datas.ribbon_index * 255.f / datas.ribbon_count;
  float s = 255.f / ((float) (datas.segment_count - datas.segment_index) / datas.segment_count); 
  float v = 255.f;

  CRGB color;
  hsv2rgb_rainbow(CHSV(h, s, v), color);
  return color;
}

struct TracerState
{
  unsigned int cptr = 0;
};
CRGB render_tracer(const RenderDatas& datas, const void* _state)
{
  const TracerState& state = *static_cast<const TracerState*>(_state);
  const unsigned int cptr_index = (state.cptr >> 3) % datas.ribbon_length;
  return cptr_index == datas.pixel_index ? CRGB::White : CRGB::Black;
}

struct TurnstileState
{

};
CRGB render_turnstile(const RenderDatas& datas, const void* _state)
{
  const TurnstileState& state = *static_cast<const TurnstileState*>(_state);
  return CRGB::Blue;
}

using Generator = CRGB (*)(const RenderDatas&, const void*);

struct Calibrator
{
  enum Mode
  {
    ColorWheel = 0,
    Tracer,
    Turnstile,
    ModeCount
  };

  unsigned int cptr = 0;
  unsigned int period = 400;
  Mode mode = ColorWheel;
  TracerState tracer;
  
  CRGB render(const RenderDatas& datas) const
  {
    static const Generator generators[Mode::ModeCount] = {render_color_wheel, render_tracer, render_turnstile};
    const void* states[Mode::ModeCount] = {0, &tracer, 0};
    return generators[mode](datas, states[mode]);
  }

  void evolve()
  {
    cptr = (cptr+1) % period;
    tracer.cptr += 1;
    if (0 == cptr)
    {
      mode = (mode +1) % Mode::ModeCount;
      tracer.cptr = 0;
    }
  }

};

struct RGBControler
{
  uint8_t red = 128;
  uint8_t green = 0;
  uint8_t blue = 0;

  CRGB render(const RenderDatas& datas) const
  {
    return CRGB(red, green, blue);
  }

  void evolve()
  {
  }
};

struct PaleteGenerator
{
  CRGB render(const RenderDatas& datas) const
  {
    float fpos = (float)datas.pixel_index / datas.ribbon_length;
    coef_t pos = coef_t(fpos);
    vec3<coef_t> color = palette.eval(pos);
    return CRGB(color.r * 255, color.g * 255, color.b * 255); 
  }
  
  ColorPalette<coef_t> palette = {
    {0.5, 0.5, 0.5}, {0.5, 0.5, 0.5}, {2.0, 1.0, 0.0}, {0.5, 0.2, 0.25}
  };
};


struct Optopoulpe_t
{
  static constexpr const unsigned int MaxTentaclesCount = 16;
  static constexpr const unsigned int MaxLedsCount = 14;
  static constexpr const unsigned int SegmentPoolSize = 64;
  
  struct Segment
  {
    static constexpr const unsigned int End = ~0; // intmax
    static Array<Segment, SegmentPoolSize> Pool;
    
    unsigned int length = 0; // Lenght in led count
    unsigned int offset = 0; // Index of segment's first led

    unsigned int next = End; // Index of the next segment or intmax (~0) if none
  };

  struct Tentacle
  {
    unsigned int first_segment = Segment::End; // Index of tentacle's first segment
    unsigned int chain_length = 0;    // Number of segments in this chain
    unsigned int virtual_length = 0;  // last segment's offset + length

    void add_segment(unsigned int length, unsigned int offset)
    {
      const unsigned int index = Segment::Pool.size;
      Segment segment;
        segment.length = length;
        segment.offset = offset;
      Segment::Pool.add(segment);

      unsigned int* tmp = 0;
      for (tmp = &first_segment ; *tmp != Segment::End ; tmp = &Segment::Pool[*tmp].next)
        {}
      *tmp = index;

      chain_length += 1;
      virtual_length += length + offset; // erreur ici si l'offset est donné en absolu et non en relatif
    }
  };

  mutable CRGB leds[MaxLedsCount];
  Array<Tentacle,MaxTentaclesCount> tentacles;

  Calibrator calibrator;
  RGBControler rgb_controler;

  PaleteGenerator palette_0;

  CRGB compute_pixel_color(const RenderDatas& datas) const
  {
    return palette_0.render(datas);
  }

  void compute_frame() const
  {
    unsigned int index = 0;
    for (unsigned int t = 0 ; t < tentacles.size ; ++t)
    {
      const Tentacle& tentacle = tentacles[t];
      unsigned int segindex = 0;
      for (
        unsigned int s = tentacle.first_segment ; 
        s != Segment::End ; 
        s = Segment::Pool[s].next, ++segindex)
      {
        const Segment& segment = Segment::Pool[s];
        for (unsigned int i = 0 ; i < segment.length ; ++i, ++index)
        {
          // Make a great function call like :

          RenderDatas datas;
            datas.pixel_index = i + segment.offset;
            datas.old_color = leds[index];

            datas.ribbon_length = tentacle.virtual_length;
            datas.ribbon_index = t;
            datas.ribbon_count = tentacles.size;

            datas.segment_index = segindex;
            datas.segment_count = tentacle.chain_length;
          leds[index] = compute_pixel_color(datas);
        }
      }
    }
  }

  void evolve()
  {
    // More complicated, something like
    // for each modifier, generator, etc... update it's parameters
    calibrator.evolve();
    rgb_controler.evolve();
  }

} Optopoulpe;

Array<Optopoulpe_t::Segment, Optopoulpe_t::SegmentPoolSize> Optopoulpe_t::Segment::Pool;
sfx::midi::Parser<128, 128> MidiParser;

void setup() {

  Serial.begin(9600);
  while (!Serial) ;

  FastLED.addLeds<NEOPIXEL, 2>(Optopoulpe.leds, Optopoulpe.MaxLedsCount);
  memset(Optopoulpe.leds, 0, sizeof(CRGB) * Optopoulpe.MaxLedsCount);
  FastLED.show();

  auto& t0 = Optopoulpe.tentacles.add(Optopoulpe_t::Tentacle());
  t0.add_segment(14,0);

  MidiParser.Init();
}

void loop() {
  Optopoulpe.compute_frame();
  FastLED.show();
  Optopoulpe.evolve();
  
  while (Serial.available()) 
  {
    int input_byte = Serial.read();
    if (input_byte < 0)
      continue;
    MidiParser.Parse(input_byte);
  }
  
  while (MidiParser.HasNext())
  {
    sfx::midi::Event event = MidiParser.NextEvent();
    if (sfx::midi::ControlChange == event.status)
    {
      if (0x07 == event.d1)
      {
        if (0 == event.channel)
          Optopoulpe.rgb_controler.red = event.d2;
        else if (1 == event.channel)
          Optopoulpe.rgb_controler.green = event.d2;
        else if (2 == event.channel)
          Optopoulpe.rgb_controler.blue = event.d2;
      }
      else if ((0x30 <= event.d1 && event.d1 <= 0x37) || (0x10 <= event.d1 && event.d1 <= 0x13))
      {
        unsigned int idx = 0;
        unsigned int coef = 0;

        if (0x30 <= event.d1 && event.d1 <= 0x37)
        {
          idx = event.d1 - 0x30;
          coef = idx / 4;
        }
        if (0x10 <= event.d1 && event.d1 <= 0x13)
        {
          idx = event.d1 - 0x10;
          coef = 2;
        }

        unsigned int param = idx % 4;
        Optopoulpe.palette_0.palette[param][coef] = event.d2 / 255.f;
      }
    }
  }
  delay(10);
}
