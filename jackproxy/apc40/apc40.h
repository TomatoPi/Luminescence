#pragma once

#include "controller.h"

#include <cassert>

namespace apc
{
  static constexpr const size_t PadsRowsCount = 5;
  static constexpr const size_t PadsColumnsCount = 8;

  static constexpr const size_t BanksCount = 9;

  static constexpr const size_t EncodersBlockSize = 8;
  static constexpr const size_t EncodersRowsCount = 4;
  static constexpr const size_t EncodersColumnsCount = 4;

  static constexpr const size_t FadersCount = 8;

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
        this,
        signature_on,
        make_callback(
          [this](){ handleOn(); }));
      controller->register_mapping(
        this,
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

  template <uint8_t CC>
  class Pot :
    public Controller::Control
  {
  protected:

    MidiMsg signature;
    uint8_t value = 0;

    void handle_message(const MidiMsg& event)
    {
      value = event.d2 << 1;
      do_handle(event);
      fprintf(stderr, "Catched\n");
    }

    virtual void do_handle(const MidiMsg& event) {}

    void register_mappings()
    {
      controller->register_mapping(this, signature, std::bind_front(&Pot::handle_message, this));
    }

    void send_refresh() override
    {
      MidiMsg tmp(signature);
      tmp.d2 = value >> 1;
      controller->push_event(tmp);
    }

  public:

    Pot(Controller* ctrl) :
      Control(ctrl),
      signature({0xb0, CC, 0})
    {
    }

    uint8_t get_value() const { return value; }
  };

  class Encoder :
    public Pot<0>
  {
  public:
    using Base = Pot<0>;
    using Instances = std::array<std::array<std::array<Encoder*,EncodersRowsCount>, EncodersColumnsCount>, BanksCount>;
  private:
    static Instances _encoders;

  public:

    Encoder(Controller* ctrl, uint8_t bank, uint8_t block, uint8_t index) :
      Base(ctrl)
    {
      signature.c = bank;
      signature.d1 = (0 == block ? 0x30 : 0x10) + index;
      _encoders[bank][index % 4][(block * 2) + (index / 4)] = this;
      register_mappings();
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

  class MainFader :
    public Pot<0x0e>
  {
  public:
    using Base = Pot<0x0e>;
  private:
    static MainFader* _instance;

    MidiMsg signature;
    uint8_t value = 0;

  public:

    void send_refresh() override {}

    MainFader(Controller* ctrl) :
      Base(ctrl)
    {
      assert(_instance == nullptr);
      _instance = this;
      register_mappings();
    }

    static MainFader* Get()
    {
      return _instance;
    }
  };

  class Fader :
    public Pot<0x07>
  {
  public:
    using Base = Pot<0x07>;
    using Instances = std::array<Fader*, FadersCount>;
  private:
    static Instances _instances;

    MidiMsg signature;
    uint8_t value = 0;

  public:

    void send_refresh() override {}

    Fader(Controller* ctrl, uint8_t index) :
      Base(ctrl)
    {
      signature.c = index;
      _instances[index] = this;
      register_mappings();
    }

    static Instances& Get()
    {
      return _instances;
    }
    static Fader* Get(uint8_t index)
    {
      return _instances[index];
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
      controller->register_mapping(this, signature, std::bind_front(&Panic::callback, this));
    }
  };

  class APC40 : public Controller
  {
  public:
    APC40()
    {
      addControl<MainFader>();
      addControl<Panic>();
      for (uint8_t fader = 0 ; fader < FadersCount ; ++fader)
        addControl<Fader>(fader);
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