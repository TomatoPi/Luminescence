#pragma once

struct Clock
{
  static constexpr const uint8_t MaxClocksCount = 16;
  static uint8_t _clockIndex;
  static Clock* _clocks[MaxClocksCount];
  uint32_t clock = 0;
  uint32_t period = 0;
  uint32_t last_timestamp = 0;

  uint32_t _dt;

  void tick(uint32_t timestamp)
  {
    clock += (timestamp - last_timestamp) * _dt;
    last_timestamp = timestamp;
  }

  void setPeriod(uint32_t period)
  {
    this->period = period << 3;
    _dt = 0xFFFFFFFFu / (((period) << 3) +1);
  }

  uint8_t get8(uint8_t exp = 0) const { return ((clock & (0xFFFFFFFFu >> (3 - exp))) << (3 - exp)) >> 24; }
  uint16_t get16(uint8_t exp = 0) const { return ((clock & (0xFFFFFFFFu >> (3 - exp))) << (3 - exp)) >> 16; }

  Clock()
  {
    _clocks[_clockIndex++] = this;
  }

  static void Tick(uint32_t timestamp) {
    for (uint8_t i = 0 ; i < _clockIndex ; ++i)
      _clocks[i]->tick(timestamp);
  }
};
