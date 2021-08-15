#pragma once

#include "controller.h"

namespace apc
{
  static constexpr const size_t PadsRowsCount = 5;
  static constexpr const size_t PadsColumnsCount = 8;

  static constexpr const size_t BanksCount = 9;

  static constexpr const size_t EncodersBlockSize = 8;
  static constexpr const size_t EncodersRowsCount = 4;
  static constexpr const size_t EncodersColumnsCount = 4;

  class Pad : 
    public Controller::Control
  {
  public:
    using Instances = std::array<std::array<Pad*,PadsRowsCount>,PadsColumnsCount>;
  private:
    static Instances _pads;

    MidiMsg signature_on;
    MidiMsg signature_off;
    bool status = false;

    void handleOn()
    {
      status = !status;
    }

    void handleOff()
    {
      send_refresh();
    }

  public:

    void send_refresh() override
    {
      controller->push_event(status ? signature_on : signature_off);
    }

    Pad(Controller* ctrl, uint8_t col, uint8_t row) :
      Controller::Control(ctrl),
      signature_on({0x90 | col, 0x35 + row, 0x7f}),
      signature_off({0x80 | col, 0x35 + row, 0x7f})
    {
      controller->register_mapping(
        signature_on,
        make_callback(
          [this](){ handleOn(); }));
      controller->register_mapping(
        signature_off,
        make_callback(
          [this](){ handleOff(); }));
      _pads[col][row] = this;
    }

    static Instances& Get()
    {
      return _pads;
    }
    static Pad* Get(uint8_t col, uint8_t row)
    {
      return _pads[col][row];
    }
  };

  class Encoder :
    public Controller::Control
  {
  public:
    using Instances = std::array<std::array<std::array<Encoder*,EncodersRowsCount>, EncodersColumnsCount>, BanksCount>;
  private:
    static Instances _encoders;

    MidiMsg signature;
    uint8_t value = 0;

    void handle_message(const MidiMsg& event)
    {
      value = event.d2;
    }

  public:

    void send_refresh() override
    {
      MidiMsg tmp(signature);
      tmp.d2 = value;
      controller->push_event(tmp);
    }

    Encoder(Controller* ctrl, uint8_t bank, uint8_t block, uint8_t index) :
      Control(ctrl),
      signature({0xb0 | bank, (0 == block ? 0x30 : 0x10) + index, 0})
    {
      controller->register_mapping(signature, std::bind_front(&Encoder::handle_message, this));
      _encoders[bank][index % 4][(block * 2) + (index / 4)] = this;
    }

    static Instances& Get()
    {
      return _encoders;
    }
    static Encoder* Get(uint8_t bank, uint8_t col, uint8_t row)
    {
      return _encoders[bank][col][row];
    }
  };

  class Panic :
    public Controller::Control
  {
  private:
    MidiMsg signature = {0x90, 0x62, 0x7f};

    void callback(const MidiMsg&) 
    {
      controller->refresh_all_controllers();
    }
  
  public:

    void send_refresh() override {}

    Panic(Controller* ctrl) : Control(ctrl)
    {
      controller->register_mapping(signature, std::bind_front(&Panic::callback, this));
    }
  };

  class APC40 : public Controller
  {
  public:
    APC40()
    {
      addControl<Panic>();
      for (uint8_t col = 0 ; col < PadsColumnsCount ; ++col)
        for (uint8_t row = 0 ; row < PadsRowsCount ; ++row)
        {
          addControl<Pad>(col, row);
        }
      for (uint8_t bank = 0 ; bank < BanksCount ; ++bank)
      {
        for (uint8_t block = 0 ; block < 2 ; ++block)
          for (uint8_t index = 0 ; index < EncodersBlockSize ; ++index)
          {
            addControl<Encoder>(bank, block, index);
          }
      }
    }
  };
}