#pragma once

#include <cstdint>
#include <vector>

/// @brief Namespace holding midi related objects
namespace midi {
  
  /// @brief Represents a raw midi message in binary format
  using raw_message = std::vector<uint8_t>;

  /// @brief Public interface for midi bridges
  class bridge {
  public :
    virtual ~bridge() noexcept = default;

    virtual void activate() = 0;

    virtual std::vector<raw_message> incomming_midi() = 0;
    virtual void send_midi(const raw_message& msg) = 0;
  };
}