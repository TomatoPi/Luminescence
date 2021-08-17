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
    using Instances = std::array<std::array<Pad*,PadsRowsCount+1>,PadsColumnsCount+1>;
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

    Pad(Controller* ctrl, uint8_t col, uint8_t row, uint8_t master, uint8_t bottom) :
      Controller::Control(ctrl),
      signature_on({0x90 | col, 0x35 + row, 0x7f}),
      signature_off({0x80 | col, 0x35 + row, 0x7f})
    {
      signature_on.c = signature_off.c = master ? 0 : col;
      signature_on.d1 = signature_off.d1 = (master ? 0x51 : 0x34) + (bottom ? 0 : row + 1);
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
      fprintf(stderr, "%d %d %02x %02x %02x %02x\n", col, row, signature_on.s, signature_on.c, signature_on.d1, signature_on.d2);
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
      signature({0xb0, CC, 0, 0})
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
      signature.s = 0xb0;
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

  public:

    void send_refresh() override {}

    MainFader(Controller* ctrl) :
      Base(ctrl)
    {
      assert(_instance == nullptr);
      signature.s = 0xb0;
      signature.c = 0;
      signature.d1 = 0x0e;
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

  public:

    void send_refresh() override {}

    Fader(Controller* ctrl, uint8_t index) :
      Base(ctrl)
    {
      signature.s = 0xb0;
      signature.c = index;
      signature.d1 = 0x07;
      register_mappings();
      _instances[index] = this;
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

  template <uint8_t CC>
  class Trigger :
    public Controller::Control
  {
  protected:
    static Trigger* _instance;
    MidiMsg signature = {0x90, CC, 0x7f};

    void handle_event(const MidiMsg& event) 
    {
      callback(event);
    }

    virtual void callback(const MidiMsg&) {}
  
  public:

    void send_refresh() override {}

    Trigger(Controller* ctrl) : Control(ctrl)
    {
      controller->register_mapping(this, signature, std::bind_front(&Trigger<CC>::handle_event, this));
      _instance = this;
    }

    static Trigger* Get() { return _instance; }
  };

  template <uint8_t CC> Trigger<CC>* Trigger<CC>::_instance = nullptr;

  class Panic : public Trigger<0x62>
  {
  private:
    virtual void callback(const MidiMsg&)
    {
      controller->refresh_all_controllers();
    }
  public:
    Panic(Controller* ctrl) : Trigger(ctrl) {}
  };

  class TapTempo : public Trigger<0x63>
  {
  public:
    TapTempo(Controller* ctrl) : Trigger(ctrl) {}
  };
  class IncTempo : public Trigger<0x64>
  {
  public:
    IncTempo(Controller* ctrl) : Trigger(ctrl) {}
  };
  class DecTempo : public Trigger<0x65>
  {
  public:
    DecTempo(Controller* ctrl) : Trigger(ctrl) {}
  };


  class SyncPot :
    public Pot<0x2f>
  {
  public:
    using Base = Pot<0x2f>;
  private:
    static SyncPot* _instance;

  public:

    void send_refresh() override {}

    SyncPot(Controller* ctrl) :
      Base(ctrl)
    {
      register_mappings();
      _instance = this;
    }

    static SyncPot* Get()
    {
      return _instance;
    }
  };

  class APC40 : public Controller
  {
  private:
    APC40()
    {
      addControl<MainFader>();
      addControl<Panic>();

      addControl<TapTempo>();
      addControl<IncTempo>();
      addControl<DecTempo>();
      addControl<SyncPot>();

      for (uint8_t fader = 0 ; fader < FadersCount ; ++fader)
        addControl<Fader>(fader);

      for (uint8_t col = 0 ; col <= PadsColumnsCount ; ++col)
      {
        for (uint8_t row = 0 ; row <= PadsRowsCount ; ++row)
        {
          addControl<Pad>(col, row, col == PadsColumnsCount, row == PadsRowsCount);
        }
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
  public:
    static APC40& Get() { static APC40 apc; return apc; }
  };
}