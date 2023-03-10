#pragma once

#include "utils/visitor.h"

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

/// @brief Namespace holding all objects related to communication
///   with display devices
namespace transport
{
  /* *** Communication objects *** */

  /// @brief Common object representing packets sent to devices
  struct packet {
    uint64_t date;
    std::vector<uint8_t> payload;
  };

  /// @brief Flag representing send-state of a packet
  ///   Failed : the message cannot be sent due to a connection error
  ///   Sent : the message has been successfully sent
  ///   Unavailable : the connection is currently unavailable
  enum class packet_status {
    Failed, Sent, Unavailable
  };
    
  /// @brief Common object representing a reply from a device
  struct reply {
    std::string content;
  };
  using opt_reply = std::optional<reply>;

  /* *** Transport public interface *** */

  /// @brief Interface for objects holding a connection to a device
  class transport {
  public:

    /// @brief Send a packet to the connected device
    /// @param p the packet to send
    /// @return a future object to keep track of packet status
    virtual packet_status send(const packet& p) = 0;

    /// @brief Look for incomming transmition from the device
    /// @return A message from the device if available
    virtual opt_reply receive() = 0;

    /// @brief Check for device availability
    /// @return true if the device is available for communication
    virtual bool alive() const noexcept = 0;
    
    virtual ~transport() = default;
  };
}