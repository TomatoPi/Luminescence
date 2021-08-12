#pragma once

#include "utils.h"

namespace opto
{
  /// System's master clock
  template <typename Time>
  struct Syncbase
  {
    const Time* clock = nullptr;   ///!< Time since the last sync
    const Time* period = nullptr;  ///!< Time between two beats
  };

  /// Clock modifier
  template <typename Time>
  struct Syncview
  {
    Time phase = 0; ///!< View offset in [0, sync->period[
    int subdivide_level = 0; ///!< Fraction of master clock taken
    const Syncbase<Time>* sync = nullptr; ///!< Master clock
    const Syncview<Time>* subsync = nullptr; ///!< Reference clock

    Time clock() const noexcept {
      Time clock = subsync ? subsync->clock() : *sync->clock;
      return clock + phase;
    }

    Time period() const noexcept {
      Time period = subsync ? subsync->period() : *sync->period;
      return subdivide_level < 0 ? period >> -subdivide_level : period << subdivide_level;
    }

    template <typename T>
    T eval() const noexcept {
      const Time p = period();
      const Time c = clock();
      return 0 == p ? T(0.f) : T(c % p) / T(p);
    }
  };


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

    using transfert_f = T(*)(T, T, T, T);

    T param1 = 0, param2 = 0;
    transfert_f tfunc = w_sin;
    T* controlled_value = nullptr;

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
  
} // namespace opto
