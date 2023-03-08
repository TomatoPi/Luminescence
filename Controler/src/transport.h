#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <future>
#include <optional>

/// @brief Namespace holding all objects related to communication
///   with display devices
namespace transport
{
  /// @brief Common object representing packets sent to devices
  struct packet {
    enum class status {
      Failed, Sent, Skipped
    };
      
    uint64_t uuid;
    std::vector<uint8_t> payload;
  };

  /// @brief Common object representing a reply from a device
  struct reply {
    std::string content;
  };

  /// @brief Interface for objects holding a connection to a device
  class transport {
  public:

    /// @brief Send a packet to the connected device
    /// @param p the packet to send
    /// @return a future object to keep track of packet status
    virtual std::future<packet::status> send(const packet& p) = 0;

    /// @brief Look for incomming transmition from the device
    /// @return A message from the device if available
    virtual std::optional<reply> receive() = 0;

    /// @brief Must stop communication with the device
    /// @param async if true, this call is non-blocking
    virtual void kill(bool async = false) = 0;

    /// @brief Check for device availability
    /// @return true if the device is available for communication
    virtual bool alive() const noexcept = 0;
    
    virtual ~transport() = default;
  };
}