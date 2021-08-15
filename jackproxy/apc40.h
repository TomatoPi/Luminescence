#pragma once

#include "midihash.h"

#include <cstdint>
#include <queue>
#include <array>

#include <stdio.h>

namespace apc
{
  static constexpr const size_t BanksCount = 8;

  struct Pads
  {
    static constexpr const size_t RowsCount = 5;
    static constexpr const size_t ColumnsCount = 8;

    struct Pad {
      MidiMsg signature_on;
      MidiMsg signature_off;
      bool state = false;

      void handleOn() { state = !state; }
      void handleOff() {}

      void register_controls(MidiHashMap& table)
      {
        table.emplace(signature_on, make_callback(
          [this](){ handleOn(); }));
        table.emplace(signature_off, [this](auto) -> auto {
          std::vector<MidiMsg> res;
          res.emplace_back(generate_update_msg());
          return res;
        });
      }

      MidiMsg generate_update_msg() const
      {
        return state ? signature_on : signature_off;
      }
    };

    std::array<std::array<Pad,RowsCount>,ColumnsCount> pads;

    Pads()
    {
      for (uint8_t col = 0 ; col < Pads::ColumnsCount ; ++col)
      {
        for (uint8_t row = 0 ; row < Pads::RowsCount ; ++row)
        {
          uint8_t raw[3] = {0x90 | col, 0x35 + row, 0x7f};
          Pads::Pad& pad = pads[col][row];
          pad.signature_on = MidiMsg(raw);
          raw[0] = 0x80 | col;
          pad.signature_off = MidiMsg(raw);
        }
      }
    }
  };

  struct Controller
  {
    Pads pads;
  };

  struct Context
  {
    MidiHashMap midi_table;
    Controller controller;

    std::queue<MidiMsg> async_queue;
    
    void MapMidiInputs()
    {
      for (auto& col : controller.pads.pads)
        for (auto& pad : col)
        {
          pad.register_controls(midi_table);
        }
    }

    Context()
    {
      for (auto& col : controller.pads.pads)
        for (auto& pad : col)
        {
          async_queue.push(pad.generate_update_msg());
        }
    }
  };


}; /* namespace apc */