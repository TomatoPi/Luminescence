#pragma once

#include "controller.h"
#include "instanced.h"
#include "controls/pad.h"
#include "controls/pot.h"

#include <cassert>

namespace apc
{
  static constexpr const size_t PadsRowsCount = 5;
  static constexpr const size_t PadsColumnsCount = 8;

  static constexpr const size_t BanksCount = 9;
  static constexpr const size_t MasterBank = 8;

  static constexpr const size_t EncodersRowsCount = 2;
  static constexpr const size_t EncodersColumnsCount = 4;

  static constexpr const size_t FadersCount = 8;

  static constexpr const size_t TracksCount = 8;

  /////////////////////////////////
  /// Pads
  /////////////////////////////////

  // Will enable - disable modulation stages on a compo
  class PadsMatrix : 
    public ctrls::TogglePad,
    public D2ArrayInstanced<PadsMatrix, PadsColumnsCount, PadsRowsCount>
  {
  private:
    using Base = ctrls::TogglePad;
    using Inst = D2ArrayInstanced<PadsMatrix, PadsColumnsCount, PadsRowsCount>;
    
    void handle_off() override { Base::handle_off(); send_refresh(); };

  public:

    PadsMatrix(Controller* ctrl, uint8_t col, uint8_t row) :
      Base(ctrl, col, 0x35 + row), Inst(col, row)
    {
    }
  };

  class SequencerPads : 
    public ctrls::MomentaryPad,
    public D2ArrayInstanced<SequencerPads, TracksCount, 3>
  {
  private:
    using Base = ctrls::MomentaryPad;
    using Inst = D2ArrayInstanced<SequencerPads, TracksCount, 3>;

  public:

    SequencerPads(Controller* ctrl, uint8_t col, uint8_t row) :
      Base(ctrl, col, 0x32 - row), Inst(col, row)
    {
    }
  };

  // We'll use this to momentary show a compo
  // or to send it as a boolean input to compo's param
  class PadsBottomRow :
    public ctrls::MomentaryPad,
    public ArrayInstanced<PadsBottomRow, PadsColumnsCount>
  {
  private:
    using Base = ctrls::MomentaryPad;
    using Inst = ArrayInstanced<PadsBottomRow, PadsColumnsCount>;

  public:

    PadsBottomRow(Controller* ctrl, uint8_t index) :
      Base(ctrl, index, 0x34), Inst(index)
      {}
  };

  // Will enable / disable global effects
  class PadsMaster :
    public ctrls::TogglePad,
    public ArrayInstanced<PadsMaster, PadsRowsCount>
  {
  private:
    using Base = ctrls::TogglePad;
    using Inst = ArrayInstanced<PadsMaster, PadsRowsCount>;
    
    void handle_off() override { Base::handle_off(); send_refresh(); };

  public:

    PadsMaster(Controller* ctrl, uint8_t index) :
      Base(ctrl, 0, 0x52 + index), Inst(index)
      {}
  };

  // Probably has nth to do, all is handle natively by the controller
  // Change edited track
  class TrackSelect :
    public ctrls::Trigger,
    public ArrayInstanced<TrackSelect, BanksCount>
  {
  private:
    using Base = ctrls::Trigger;
    using Inst = ArrayInstanced<TrackSelect, BanksCount>;

    bool is_active = false;

    void handle_event() override {
      Base::handle_event();
      for (auto& pad : Get())
        pad->is_active = (pad == this);
    }

  public:

    TrackSelect(Controller* ctrl, uint8_t index) :
      Base(ctrl, index, 0x17, 0xB0), Inst(index)
      {}

    bool isActive() const { return is_active; }

    static TrackSelect* GetMaster() { return Get(MasterBank); }
  };

  ////////////////////////////////
  /// Encoders
  ////////////////////////////////

  // Used to edit LFOs values, or kinda global params
  class StrobeParams :
    public ctrls::Pot,
    public ArrayInstanced<StrobeParams, EncodersColumnsCount>
  {
  public:
    using Base = ctrls::Pot;
    using Inst = ArrayInstanced<StrobeParams, EncodersColumnsCount>;
  
  public:

    StrobeParams(Controller* ctrl, uint8_t index) :
      Base(ctrl, 0, 0x30 + index),
      Inst(index)
    {
    }
  };

  class StrobeEnable :
    public ctrls::MomentaryPad,
    public MonoInstanced<StrobeEnable>
  {
  private:
    using Base = ctrls::MomentaryPad;
    using Inst = MonoInstanced<StrobeEnable>;

  public:

    StrobeEnable(Controller* ctrl) :
      Base(ctrl, 0, 0x57), Inst()
      {}
  };

  class OscParams :
    public ctrls::Pot,
    public D2ArrayInstanced<OscParams, 3, EncodersColumnsCount>
  {
  public:
    using Base = ctrls::Pot;
    using Inst = D2ArrayInstanced<OscParams, 3, EncodersColumnsCount>;
  
  public:

    OscParams(Controller* ctrl, uint8_t osc, uint8_t param) :
      Base(ctrl, 0, 0x34 + param),
      Inst(osc, param)
    {
    }
  };

  class OscSelect :
    public ctrls::Trigger,
    public ArrayInstanced<OscSelect, 3>
  {
  private:
    using Base = ctrls::Trigger;
    using Inst = ArrayInstanced<OscSelect, 3>;

    bool is_active = false;

    void handle_event() override {
      Base::handle_event();
      for (auto& osc : Get())
          osc->is_active = (osc == this);
      send_refresh();
      OscParams::Generate([this](uint8_t bank, uint8_t col){
        if (bank == this->getIndex())
          OscParams::Get(bank, col)->send_refresh();
      });
    }

  public:

    OscSelect(Controller* ctrl, uint8_t index) :
      Base(ctrl, index, 0x58), Inst(index)
      {}

    bool isActive() const { return is_active; }

  };



  // Edit Current compo's params
  class BottomEncoders :
    public ctrls::Pot,
    public D3ArrayInstanced<BottomEncoders, BanksCount, EncodersColumnsCount, EncodersRowsCount>
  {
  public:
    using Base = ctrls::Pot;
    using Inst = D3ArrayInstanced<BottomEncoders, BanksCount, EncodersColumnsCount, EncodersRowsCount>;
  
  public:

    BottomEncoders(Controller* ctrl, uint8_t bank, uint8_t col, uint8_t row) :
      Base(ctrl, bank, 0x10 + (row * 4) + col),
      Inst(bank, col, row)
    {
    }

    static Inst::Bank& GetMaster() { return Get(MasterBank); } 
    static Inst::Column& GetMaster(uint8_t col) { return Get(MasterBank, col); } 
    static BottomEncoders* GetMaster(uint8_t col, uint8_t row) { return Get(MasterBank, col, row); } 
  };

  //////////////////////////////////////
  /// Faders
  //////////////////////////////////////

  // A - B panning between two things ?
  class Faders :
    public ctrls::Pot,
    public ArrayInstanced<Faders, FadersCount>
  {
  public:
    using Base = ctrls::Pot;
    using Inst = ArrayInstanced<Faders, FadersCount>;

  public:

    Faders(Controller* ctrl, uint8_t index) :
      Base(ctrl, index, 0x07),
      Inst(index)
    {
    }
  };

  // Global brightness
  class MainFader :
    public ctrls::Pot,
    public MonoInstanced<MainFader>
  {
  public:
    using Base = ctrls::Pot;
    using Inst = MonoInstanced<MainFader>;

  public:

    MainFader(Controller* ctrl) :
      Base(ctrl, 0, 0x0e), Inst()
    {
    }
  };

  /////////////////////////////////////
  /// Triggers
  /////////////////////////////////////

  template <typename T, uint8_t D1>
  class MonoTrigger :
    public ctrls::Trigger,
    public MonoInstanced<T>
  {
  public:
    MonoTrigger(Controller* ctrl) : 
      ctrls::Trigger(ctrl, 0, D1),
      MonoInstanced<T>()
    {
    }
  };

  class Panic : public MonoTrigger<Panic, 0x62>
  {
  protected:
    void handle_event() override
    {
      controller->refresh_all_controllers();
    }
  public:
    Panic(Controller* ctrl) : MonoTrigger<Panic, 0x62>(ctrl) {}
  };

  class TapTempo : public MonoTrigger<TapTempo, 0x63>
  {
  public:
    TapTempo(Controller* ctrl) : MonoTrigger<TapTempo, 0x63>(ctrl) {}
  };
  class IncTempo : public MonoTrigger<IncTempo, 0x64>
  {
  public:
    IncTempo(Controller* ctrl) : MonoTrigger<IncTempo, 0x64>(ctrl) {}
  };
  class DecTempo : public MonoTrigger<DecTempo, 0x65>
  {
  public:
    DecTempo(Controller* ctrl) : MonoTrigger<DecTempo, 0x65>(ctrl) {}
  };

  // Used to adjust the bpm
  class SyncPot :
    public ctrls::Pot,
    public MonoInstanced<SyncPot>
  {
  public:
    using Base = ctrls::Pot;
    using Inst = MonoInstanced<SyncPot>;

  public:

    SyncPot(Controller* ctrl) :
      Base(ctrl, 0, 0x2F),
      Inst()
    {
    }
  };

  class APC40 : public Controller
  {
  private:
    APC40()
    {
      SequencerPads::Generate([this](uint8_t i, uint8_t j){addControl<SequencerPads>(i,j);});

      PadsMatrix::Generate([this](uint8_t i, uint8_t j){addControl<PadsMatrix>(i,j);});
      PadsBottomRow::Generate([this](uint8_t i){addControl<PadsBottomRow>(i);});
      PadsMaster::Generate([this](uint8_t i){addControl<PadsMaster>(i);});

      TrackSelect::Generate([this](uint8_t i){addControl<TrackSelect>(i);});

      StrobeParams::Generate([this](uint8_t i){addControl<StrobeParams>(i);});
      addControl<StrobeEnable>();

      OscParams::Generate([this](uint8_t i, uint8_t j){addControl<OscParams>(i,j);});
      OscSelect::Generate([this](uint8_t i){addControl<OscSelect>(i);});

      BottomEncoders::Generate([this](uint8_t i, uint8_t j, uint8_t k){addControl<BottomEncoders>(i,j,k);});

      Faders::Generate([this](uint8_t i){addControl<Faders>(i);});
      addControl<MainFader>();

      addControl<Panic>();

      addControl<TapTempo>();
      addControl<IncTempo>();
      addControl<DecTempo>();
      addControl<SyncPot>();
    }
  public:
    static APC40& Get() { static APC40 apc; return apc; }
  };
}