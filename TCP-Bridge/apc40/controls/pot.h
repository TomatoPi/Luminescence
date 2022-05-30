#pragma once

#include "../controller.h"
#include <functional>

namespace ctrls
{

  class Pot : public Controller::Control
  {
  private:
    using Base = Controller::Control;
    
  protected:

    MidiMsg signature;
    uint8_t value = 0;

    void push_refresh(bool force) override
    {
      if (force)
      {
        MidiMsg tmp = signature;
        tmp.d2 = value;
        controller->push_event(tmp);
      }
    }

    virtual void handle_message(const MidiMsg& event)
    {
      value = event.d2;
    }

  public:

    Pot(Controller* ctrl, uint8_t channel, uint8_t d1) :
      Base(ctrl),
      signature({0xb0 | channel, d1, 0})
    {
      controller->register_mapping(this, signature, std::bind(&Pot::handle_message, this, std::placeholders::_1));
    }

    uint8_t get_value() const { return value; }
    void set_value(uint8_t v) { value = v; }
  };
}