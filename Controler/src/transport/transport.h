#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

/// @brief Namespace holding all objects related to communication
///   with display devices
namespace opto::inline transport
{
  /* *** To Device Communication objects *** */

  /// @brief Common object representing packets sent to devices
  using packet_payload = std::vector<uint8_t>;

  /// @brief Flag representing send-state of a packet
  enum class packet_status {
    Failed, Sent
  };

  /* *** From Device Communication objects *** */
    
  /// @brief Common object representing a reply from a device
  struct reply {
    std::string content;
  };
  using opt_reply = std::optional<reply>;

  /* *** Exceptions *** */

  /// @brief Exception raised when the method send or receive is called
  ///   on a transport which alive() method returns false
  struct bad_transport : std::exception {};
  
  /* *** Transport public interface *** */

  /// @brief Interface for objects holding a connection to a device
  class transport {
  public:

    /// @brief Send a packet to the connected device
    /// @param p the packet to send
    /// @return a future object to keep track of packet status
    packet_status send(const packet_payload& p)
    {
      if (!this->alive())
        throw bad_transport();
      return this->vsend(p);
    }

    /// @brief Look for incomming transmition from the device
    /// @return A message from the device if available
    opt_reply receive()
    {
      if (!this->alive())
        throw bad_transport();
      return this->vreceive();
    }

    /// @brief Check for device availability
    /// @return true if the device is available for communication
    virtual bool alive() const noexcept = 0;
    
    virtual ~transport() = default;

  protected :

    /// @brief Virtual method used to effectivly send the packet
    virtual packet_status vsend(const packet_payload& p) = 0;
    
    /// @brief Virtual method used to effectivly look for pending reply 
    virtual opt_reply vreceive() = 0;
  };
}
