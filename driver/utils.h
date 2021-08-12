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

namespace opto 
{
  /**
   * Float absolute value
   **/
  template <typename T>
  static inline T abs(T a)
  {
      return (a < T(0)) ? -a : a;
  }

  /**
   * Return random T between 0 and 1
   **/
  template <typename T>
  static inline T frand()
  {
    const uint16_t max = ~((uint16_t)0);
    return (T) random(max) / (T) max;
  }

   /**
   * Return random T between min and max
   **/
  template <typename T>
  static inline T frand(T min, T max)
  {
      if (max < min)
      {
          T a = min;
          min = max;
          max = a;
      }
      return frand<T>() * (max - min) + min;
  }

  /**
   * Sinwave
   **/
  template <typename T>
  static inline T w_sin(T in, T s, T p1, T p2)
  {
    return (T)(s * sin(M_PI * 2 * in));
  }

  /**
   * Square wave
   **/
  template <typename T>
  static inline T w_sqr(T in, T s, T p1, T p2)
  {
    return (T)((in - p1 < 0) ? s * (-1) : s);
  }

  /**
   * Triangular wave
   **/
  template <typename T>
  static inline T w_tri(T in, T s, T p1, T p2)
  {
    return (T)(s * (2.0 * sfx::abs(2.0 * in - 1.0) - 1.0));
  }

  /**
   * Sawtooth
   **/
  template <typename T>
  static inline T w_saw(T in, T s, T p1, T p2)
  {
    return (T)(s * (2.0 * in - 1.0));
  }

  /**
   * Varislope
   **/
  template <typename T>
  static inline T w_var(T in, T s, T p1, T p2)
  {
    if (in < p1)
    {
      return (T)(s * (2.0 * ((p2 * in) / p1) - 1.0));
    }
    else
    {
      return (T)(s * (1.0 - 2.0 * p2 * (in - p1) / (1 - p1)));
    }
  }

  /**
   * N-Phase
   **/
  template <typename T>
  static inline T w_nph(T in, T s, T p1, T p2)
  {
    return (T)(s * sin(M_PI * 2.0 * (in + (((T) ((int) (in * (p1 + 1)))) / p2))));
  }

  /**
   * White Noise
   **/
  template <typename T>
  static inline T w_whi(T in, T s, T p1, T p2)
  {
    return (T)(frand<T>(-1.0, 1.0));
  }

  /**
   * Constant
   **/
  template <typename T>
  static inline T w_cst(T in, T s, T p1, T p2)
  {
    return (T)p1;
  }

} /* namespace opto */