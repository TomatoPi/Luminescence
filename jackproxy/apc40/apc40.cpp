#include "apc40.h"

namespace apc
{
  Pad::Instances Pad::_pads = Pad::Instances();
  Encoder::Instances Encoder::_encoders = Encoder::Instances();
  MainFader* MainFader::_instance = nullptr;
  Fader::Instances Fader::_instances = Fader::Instances();
  SyncPot* SyncPot::_instance = nullptr;
}