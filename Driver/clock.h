#pragma once

template <typename T, uint8_t _Size>
struct Instanced
{
  static constexpr const uint8_t Size = _Size;
  static uint8_t _index;
  static T* _instances[_Size];

  Instanced() {_instances[_index++] = (T*)this;}
};
template <typename T, uint8_t _Size> uint8_t Instanced<T, _Size>::_index;
template <typename T, uint8_t _Size>  T* Instanced<T, _Size>::_instances[_Size];

struct Clock : public Instanced<Clock, 16>
{
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

  static void Tick(uint32_t time) {
    for (uint8_t i = 0 ; i < _index ; ++i)
      _instances[i]->tick(time);
  }
};

struct FallDetector : public Instanced<FallDetector, 16>
{
  Clock* clock = nullptr;
  uint16_t last_value = 0;
  bool trigger = false;
  
  void tick()
  {
    uint16_t time = clock->get16();
    if (trigger == true)
      trigger = false;
    if (time < last_value)
      trigger = true;
    last_value = time;
  }

  void reset()
  {
    trigger = false;
  }

  static void Tick() {
    for (uint8_t i = 0 ; i < _index ; ++i)
      _instances[i]->tick();
  }
};

struct FastClock : public Instanced<FastClock, 16>
{
  uint8_t clock = 0;
  uint8_t period = 0;
  uint8_t coarse_value = 0;
  uint8_t finevalue = 0;

  void tick()
  {
    clock = (clock +1) % period;
    coarse_value = clock == 0;
    finevalue = ((uint16_t)clock * 255) / period;
  }

  void setPeriod(uint8_t frames)
  {
    period = frames;
  }

  static void Tick() {
    for (uint8_t i = 0 ; i < _index ; ++i)
      _instances[i]->tick();
  }
};
