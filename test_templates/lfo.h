/*
 * Copyright (C) 2018 Space-Computer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   LFO.h
 * Author: Space-Computer
 *
 * Created on 11 août 2018, 02:37
 */

#pragma once

#include "types.h"

namespace sfx {
  using sample_t = coef_t;
  /**
   * Float absolute value
   **/
  static inline float abs(float a)
  {
      return (a < 0) ? -a : a;
  }

  /**
   * Return random float between 0 and 1
   **/
  static inline float frand()
  {
    const uint32_t max = ~((uint32_t)0);
    return (float) random(max) / (float) max;
  }

   /**
   * Return random float between min and max
   **/
  static inline float frand(float min, float max)
  {
      if (max < min)
      {
          float a = min;
          min = max;
          max = a;
      }
      return frand()*(max - min) + min;
  }
}

template <typename T>
struct LFO
{
  enum WaveForm
  {
      WSIN = 0, /**< Sinwave */
      WSQR = 1, /**< Square */
      WTRI = 2, /**< Triangular */
      WSAW = 3, /**< Sawtooth */
      WVAR = 4, /**< Varislope */
      WNPH = 5, /**< N-Phase */
      WWHI = 6, /**< White noise */
      WCST = 7, /**< Constant */
      Count
  } waveform;

  /**
   * Sinwave
   **/
  static inline sfx::sample_t w_sin(sfx::sample_t in, float s, float p1, float p2)
  {
    return (sfx::sample_t)(s * sin(M_PI * 2 * in));
  }

  /**
   * Square wave
   **/
  static inline sfx::sample_t w_sqr(sfx::sample_t in, float s, float p1, float p2)
  {
    return (sfx::sample_t)((in - 0.5 < 0) ? s * (-1) : s);
  }

  /**
   * Triangular wave
   **/
  static inline sfx::sample_t w_tri(sfx::sample_t in, float s, float p1, float p2)
  {
    return (sfx::sample_t)(s * (2.0 * sfx::abs(2.0 * in - 1.0) - 1.0));
  }

  /**
   * Sawtooth
   **/
  static inline sfx::sample_t w_saw(sfx::sample_t in, float s, float p1, float p2)
  {
    return (sfx::sample_t)(s * (2.0 * in - 1.0));
  }

  /**
   * Varislope
   **/
  static inline sfx::sample_t w_var(sfx::sample_t in, float s, float p1, float p2)
  {
    if (in < p1)
    {
      return (sfx::sample_t)(s * (2.0 * ((p2 * in) / p1) - 1.0));
    }
    else
    {
      return (sfx::sample_t)(s * (1.0 - 2.0 * p2 * (in - p1) / (1 - p1)));
    }
  }

  /**
   * N-Phase
   **/
  static inline sfx::sample_t w_nph(sfx::sample_t in, float s, float p1, float p2)
  {
    return (sfx::sample_t)(s * sin(M_PI * 2.0 * (in + (((float) ((int) (in * (p1 + 1)))) / p2))));
  }

  /**
   * White Noise
   **/
  static inline sfx::sample_t w_whi(sfx::sample_t in, float s, float p1, float p2)
  {
    return (sfx::sample_t)(sfx::frand(-1.0, 1.0));
  }

  /**
   * Constant
   **/
  static inline sfx::sample_t w_cst(sfx::sample_t in, float s, float p1, float p2)
  {
    return (sfx::sample_t)p1;
  }

  typedef sfx::sample_t(*transfert_f)(sfx::sample_t, float, float, float);

  sfx::sample_t param1 = 0, param2 = 0;
  transfert_f tfunc = w_sin;

  ////////////////////////////////////////////////////////////////////
  // Fonctions d'ondes
  ////////////////////////////////////////////////////////////////////

  coef_t eval(coef_t t) const {
    return (tfunc(t, 1., param1, param2) + 1.) / 2;
  }

  static inline transfert_f castWaveform(WaveForm wave)
  {
    if (wave == WSIN) return w_sin;
    if (wave == WSQR) return w_sqr;
    if (wave == WTRI) return w_tri;
    if (wave == WSAW) return w_saw;
    if (wave == WVAR) return w_var;
    if (wave == WNPH) return w_nph;
    if (wave == WWHI) return w_whi;
    if (wave == WCST) return w_cst;

    return w_sin;
  }
};
