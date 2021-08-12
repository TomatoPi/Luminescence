#pragma once

#include "sync.h"

namespace opto
{
  template <typename Color, typename Size, typename Time>
  struct RenderDatas
  {
    mutable Color*  ribbon = nullptr;
    const Syncbase<Time>* sync = nullptr;
    
    struct Context
    {
      Size pixel_index = 0;
      Size pixel_postlast_index = 0;
      Size pixel_count = 0;

      Size ribbon_length = 0;
      Size ribbon_index = 0;
      Size ribbon_count = 0;

      Size segment_index = 0;
      Size segment_count = 0;

      Syncview<Time> time;
    };

    Context         frame_context;
    mutable Context local_context;
    
    Context& operator-> () const noexcept { return local_context; }
    Context& operator* () const noexcept { return local_context; }
  };

  template <typename Color, typename Size, typename Time>
  struct Effect
  {
    using RenderDatas = RenderDatas<Color, Size, Time>;
    using effect_t = void (*)(
      const RenderDatas& datas, 
      void* _chain_begin, 
      void* _subchain,
      void* _chain_end);
    
    static void fill_black(const RenderDatas& datas) noexcept
    {
      for (
        Size i = datas.local_context.pixel_index;
        i < datas.local_context.pixel_postlast_index;
        ++i)
      {
        datas.ribbon[i] = Color(0); 
      }
    }
  };

} /* namespace opto */
