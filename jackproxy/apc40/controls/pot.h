#pragma once

#include "../controller.h"

namespace ctrls
{

  class Pot : public Controller::Control
  {
  private:
    using Base = Controller::Control;
    
  protected:

    MidiMsg signature;
    uint8_t value = 0;

    virtual void handle_message(const MidiMsg& event)
    {
      value = event.d2 << 1;
    }

  public:

    Pot(Controller* ctrl, uint8_t channel, uint8_t d1) :
      Base(ctrl),
      signature({0xb0 | channel, d1, 0})
    {
      controller->register_mapping(this, signature, std::bind_front(&Pot::handle_message, this));
    }

    uint8_t get_value() const { return value; }
  };
}