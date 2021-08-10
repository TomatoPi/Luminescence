#pragma once

using coef_t = float;

struct Timebase
{
  unsigned long clock = 0;
  unsigned long length = ~(0ul);
  int subdivide_level = 0;
  
  coef_t phase = coef_t(0.);
  coef_t offset = coef_t(0.);

  unsigned long last_hit = 0;

  template <typename T>
  constexpr T eval(T max = T(0), T min = T(0))
  {
    return T((max - min) * phase);
  }

  constexpr unsigned long vlength(int subdivide) const
  {
    return subdivide < 0 ? length >> -subdivide : length << subdivide;
  }

  constexpr coef_t compute_phase(int subdivide) const
  {
    return vlength(subdivide_level + subdivide) == 0 
      ? 
      coef_t(1.0) 
      : 
      coef_t(clock % vlength(subdivide_level + subdivide)) 
        / coef_t(vlength(subdivide_level + subdivide));
  }

  void evolve(unsigned long t)
  {
    clock = t - last_hit;
    phase = fmod(compute_phase(0) + offset, 1.);
  }

  void hit(unsigned long t)
  {
    length = t - last_hit;
    clock = 0;

    phase = coef_t(0.0);

    last_hit = t;
    subdivide_level = 0;
  }

  void subdivide()
  {
    subdivide_level += 1;
    clock = phase * vlength(subdivide_level);
  }

  void unsubdivide()
  {
    subdivide_level -= 1;
    clock = phase * vlength(subdivide_level);
  }
};

struct RenderDatas {
  unsigned int pixel_index = 0;
  CRGB old_color = CRGB::Black;

  unsigned int ribbon_length = 0;
  unsigned int ribbon_index = 0;
  unsigned int ribbon_count = 0;

  unsigned int segment_index = 0;
  unsigned int segment_count = 0;

  const Timebase* timebase = nullptr;
};
