#pragma once

struct Fixed
{
  static constexpr uint32_t IntMax = ~static_cast<uint32_t>(0);

  union {
    uint32_t raw;
    uint32_t val;
    uint32_t v;
  };

  constexpr Fixed(uint32_t raw = 0) : raw(raw) {}
  constexpr Fixed(float f = 0.f) : raw(static_cast<uint32_t>(f * IntMax)) {}
  constexpr Fixed(double f = 0.f) : raw(static_cast<uint32_t>(f * IntMax)) {}

  explicit constexpr operator float() const {
    return static_cast<float>(val) / static_cast<float>(IntMax);
  }
  explicit constexpr operator uint8_t() const {
    return raw >> 24;
  }

  constexpr Fixed operator + (Fixed f) const
  {
    return Fixed (v + f.v);
  }
  constexpr Fixed operator - (Fixed f) const
  {
    return Fixed (v - f.v);
  }
  constexpr Fixed operator * (Fixed f) const
  {
    return Fixed (
      static_cast<uint32_t>((static_cast<uint64_t>(v) * static_cast<uint64_t>(f.v)) >> 32)
    );
  }
  constexpr Fixed operator / (Fixed f) const
  {
    return Fixed (
      static_cast<uint32_t>(static_cast<uint64_t>(v) * (static_cast<uint64_t>(1) << 32)) / f.v
    );
  }
};
