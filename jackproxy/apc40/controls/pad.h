#pragma once

#include "../controller.h"

namespace ctrls
{
  class Pad : public Controller::Control
  {
  private:
    using Base = Controller::Control;

  protected:

    MidiMsg signature_on;
    MidiMsg signature_off;
    bool status = false;

    virtual void handle_on() {};
    virtual void handle_off() {};
    
    void send_refresh() override
    {
      Base::send_refresh();
      controller->push_event(status ? signature_on : signature_off);
    }

  public:


    Pad(Controller* ctrl, uint8_t channel, uint8_t d1) :

      Base(ctrl),

      signature_on({0x90 | channel, d1, 0}),
      signature_off({0x80 | channel, d1, 0})
    {
      controller->register_mapping(
        this,
        signature_on,
        make_callback(
          [this](){ this->handle_on(); }));
      controller->register_mapping(
        this,
        signature_off,
        make_callback(
          [this](){ this->handle_off(); }));
    }

    bool get_status() const { return status; }
  };

  class MomentaryPad : public Pad
  {
  private:
    using Base = Pad;

  protected:

    void handle_on() override { Base::handle_on(); status = true; }
    void handle_off() override { Base::handle_off(); status = false; }
  
  public:

    MomentaryPad(Controller* ctrl, uint8_t channel, uint8_t d1) :
      Pad(ctrl, channel, d1)
    {
    }
  };

  class TogglePad : public Pad
  {
  private:
    using Base = Pad;

  protected:

    void handle_on() override { Base::handle_off(); status = !status; }
    void handle_off() override { Base::handle_off(); }
  
  public:

    TogglePad(Controller* ctrl, uint8_t channel, uint8_t d1) :
      Pad(ctrl, channel, d1)
    {
    }
  };

  class Trigger : public Controller::Control
  {
  protected:
    MidiMsg signature;

    virtual void handle_event() {}

  public:

    Trigger(Controller* ctrl, uint8_t channel, uint8_t d1) : 
      Control(ctrl), signature({0x90 | (channel & 0x0F), d1, 0})
    {
      controller->register_mapping(this, signature, make_callback([this](){ this->handle_event(); }));
    }
  };
}