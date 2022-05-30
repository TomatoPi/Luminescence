/*
* Unused file
*/
#pragma once

template <typename index_t, typename coef_t>
struct range_t
{
  index_t begin = index_t(0);  ///!< Index of first pixel to render
  index_t length = index_t(0); ///!< Number of pixels to render
  
  coef_t t = coef_t(0.f);   ///!< first pixel relative position in the ribbon
  coef_t dt = coef_t(0.f);  ///!< step between each pixel

  constexpr range_t(index_t b, index_t l, coef_t t, coef_t dt)
    : begin(b), length(l), t(t), dt(dt) {}

  static range_t map_on_pixel_index(
    index_t first_pixel,  /// first pixel index in global ribbon
    index_t range_length, /// range length in pixels
    index_t total_length, /// global ribbon's length
    coef_t  offset) /// offset applied to first pixel position
  {
    return range_t {
      first_pixel,
      range_length,
      fmod(((coef_t(first_pixel) / coef_t(total_length)) + offset), coef_t(1.f)),
      coef_t(1.f) / coef_t(total_length)
    };
  }
};
