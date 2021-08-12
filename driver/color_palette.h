#pragma once

#include "vec3.h"
#include "range.h"

// From https://iquilezles.org/www/articles/palettes/palettes.htm

template <
  typename color_t,
  typename pixel_index_t,
  typename coef_t>
struct color_pallette_t 
{
  using Range = range_t<pixel_index_t, coef_t>;
  using Vec3 = vec3<coef_t>;

  vec3<coef_t> eval(coef_t t) const
  {
    return a + b * cos((c * t + d) * static_cast<coef_t>(6.28f));
  }

  void eval_range(color_t* ribbon, const Range& range) const
  {
    pixel_index_t pixel = range.begin;
    coef_t t = range.t;
    for (
      pixel_index_t i = 0 ;
      i < range.length ;
      ++i, ++pixel, t = fmod((t + range.dt), coef_t(1.f)))
    {
      auto tmp = eval(t);
      ribbon[pixel] = color_t(255 * tmp.r, 255 * tmp.g, 255 * tmp.b);
    }
  }

  operator vec3<coef_t>*() { return (vec3<coef_t>*)this; }

  vec3<coef_t> a;
  vec3<coef_t> b;
  vec3<coef_t> c;
  vec3<coef_t> d;
};
