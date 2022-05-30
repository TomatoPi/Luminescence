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

  public:

    PadsMatrix(Controller* ctrl, uint8_t col, uint8_t row) :
      Base(ctrl, col, 0x35 + row), Inst(col, row)
    {
    }
  };

  class EffectsSwitches : 
    public ctrls::MomentaryPad,
    public D2ArrayInstanced<EffectsSwitches, BanksCount, EncodersColumnsCount>
  {
  private:
    using Base = ctrls::MomentaryPad;
    using Inst = D2ArrayInstanced<EffectsSwitches, BanksCount, EncodersColumnsCount>;

  public:

    EffectsSwitches(Controller* ctrl, uint8_t col, uint8_t row) :
      Base(ctrl, col, 0x3a + row), Inst(col, row)
    {
    }
  };

  class GroupSelectPads : 
    public ctrls::MomentaryPad,
    public D2ArrayInstanced<GroupSelectPads, TracksCount, 3>
  {
  private:
    using Base = ctrls::MomentaryPad;
    using Inst = D2ArrayInstanced<GroupSelectPads, TracksCount, 3>;

    void handle_on() override {
      Base::handle_on();
      for (size_t i = 0; i < 3; ++i)
        if (i != getRow())
        {
          auto other = GroupSelectPads::Get(getCol(), i);
          other->handle_off();
          other->push_refresh(true);
        }
    }

  public:

    GroupSelectPads(Controller* ctrl, uint8_t col, uint8_t row) :
      Base(ctrl, col, 0x32 - row), Inst(col, row)
    {
    }
  };

  // We'll use this to momentary show a compo
  // or to send it as a boolean input to compo's param
  class PadsBottomRow :
    public ctrls::TogglePad,
    public ArrayInstanced<PadsBottomRow, PadsColumnsCount>
  {
  private:
    using Base = ctrls::TogglePad;
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

  class GlobalEncoders : 
    public ctrls::Pot,
    public D2ArrayInstanced<GlobalEncoders, EncodersColumnsCount, EncodersRowsCount>
  {
    using Base = ctrls::Pot;
    using Inst = D2ArrayInstanced<GlobalEncoders, EncodersColumnsCount, EncodersRowsCount>;
    
  public:
    GlobalEncoders(Controller* ctrl, size_t col, size_t row) :
      Base(ctrl, 0, 0x30 + col + row * EncodersColumnsCount),
      Inst(col, row)
    {
    }
  };

  // // Used to edit LFOs values, or kinda global params
  // class StrobeParams :
  //   public ctrls::Pot,
  //   public ArrayInstanced<StrobeParams, EncodersColumnsCount>
  // {
  // public:
  //   using Base = ctrls::Pot;
  //   using Inst = ArrayInstanced<StrobeParams, EncodersColumnsCount>;
  
  // public:

  //   StrobeParams(Controller* ctrl, uint8_t index) :
  //     Base(ctrl, 0, 0x30 + index),
  //     Inst(index)
  //   {
  //   }
  // };

  // class StrobeEnable :
  //   public ctrls::MomentaryPad,
  //   public MonoInstanced<StrobeEnable>
  // {
  // private:
  //   using Base = ctrls::MomentaryPad;
  //   using Inst = MonoInstanced<StrobeEnable>;

  // public:

  //   StrobeEnable(Controller* ctrl) :
  //     Base(ctrl, 0, 0x57), Inst()
  //     {}
  // };

  // class OscParams :
  //   public ctrls::Pot,
  //   public D2ArrayInstanced<OscParams, 3, EncodersColumnsCount>
  // {
  // public:
  //   using Base = ctrls::Pot;
  //   using Inst = D2ArrayInstanced<OscParams, 3, EncodersColumnsCount>;

  //   void handle_message(const MidiMsg& event) override;
  
  // public:

  //   OscParams(Controller* ctrl, uint8_t osc, uint8_t param) :
  //     Base(ctrl, 0, 0x34 + param),
  //     Inst(osc, param)
  //   {
  //   }
  // };

  // class OscSelect :
  //   public ctrls::TogglePad,
  //   public ArrayInstanced<OscSelect, 3>
  // {
  // private:
  //   using Base = ctrls::TogglePad;
  //   using Inst = ArrayInstanced<OscSelect, 3>;

  //   void handle_on() override {
  //     for (auto& osc : Get())
  //     {
  //       osc->status = (osc == this);
  //       osc->send_refresh();
  //     }
  //     OscParams::Generate([this](uint8_t bank, uint8_t col){
  //       if (bank == this->getIndex())
  //         OscParams::Get(bank, col)->send_refresh();
  //     });
  //   }

  // public:

  //   OscSelect(Controller* ctrl, uint8_t index) :
  //     Base(ctrl, 0, 0x58 + index), Inst(index)
  //     {}
  // };

  // void OscParams::handle_message(const MidiMsg& msg)
  // {
  //   if (OscSelect::Get(getCol())->get_status())
  //   {
  //     Base::handle_message(msg);
  //     send_refresh();
  //   }
  // }


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

  class StrobeFader :
    public ctrls::Pot,
    public MonoInstanced<StrobeFader>
  {
  public:
    using Base = ctrls::Pot;
    using Inst = MonoInstanced<StrobeFader>;

  public:

    StrobeFader(Controller* ctrl) :
      Base(ctrl, 0, 0x0f), Inst()
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

  class Save : public MonoTrigger<Save, 0x5d>
  {
  public:
    Save(Controller* ctrl) : MonoTrigger<Save, 0x5d>(ctrl) {}
  };
  class Load : public MonoTrigger<Load, 0x5b>
  {
  public:
    Load(Controller* ctrl) : MonoTrigger<Load, 0x5b>(ctrl) {}
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
      GroupSelectPads::Generate([this](uint8_t i, uint8_t j){addControl<GroupSelectPads>(i,j);});

      PadsMatrix::Generate([this](uint8_t i, uint8_t j){addControl<PadsMatrix>(i,j);});
      PadsBottomRow::Generate([this](uint8_t i){addControl<PadsBottomRow>(i);});
      PadsMaster::Generate([this](uint8_t i){addControl<PadsMaster>(i);});

      TrackSelect::Generate([this](uint8_t i){addControl<TrackSelect>(i);});

      GlobalEncoders::Generate([this](uint8_t i, uint8_t j)
      {addControl<GlobalEncoders>(i,j);});
      BottomEncoders::Generate([this](uint8_t i, uint8_t j, uint8_t k){addControl<BottomEncoders>(i,j,k);});
      EffectsSwitches::Generate([this](uint8_t i, uint8_t j){addControl<EffectsSwitches>(i,j);});

      Faders::Generate([this](uint8_t i){addControl<Faders>(i);});
      addControl<MainFader>();
      addControl<StrobeFader>();

      addControl<Panic>();

      addControl<TapTempo>();
      addControl<IncTempo>();
      addControl<DecTempo>();
      addControl<SyncPot>();

      addControl<Save>();
      addControl<Load>();
    }
  public:
    static APC40& Get() { static APC40 apc; return apc; }
  };
}