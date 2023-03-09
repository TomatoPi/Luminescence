#pragma once

#include "utils/visitor.h"

#include <cstdint>
#include <vector>
#include <string>
#include <optional>

#include <chrono>
#include <future>
#include <variant>

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
  enum class packet_status {
    Failed, Sent, Expired
  };
    
  /// @brief Common object representing a reply from a device
  struct reply {
    std::string content;
  };

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
    virtual std::optional<reply> receive() = 0;

    /// @brief Must stop communication with the device
    /// @param async if true, this call is non-blocking
    virtual void kill() = 0;

    /// @brief Check for device availability
    /// @return true if the device is available for communication
    virtual bool alive() const noexcept = 0;
    
    virtual ~transport() = default;
  };

  /* *** Asynchronous transport support objects *** */

  /// @brief Object holding parameters for a blocking call of a normaly non blocking function
  struct blocking_launch {
    std::chrono::microseconds polling_rate = std::chrono::microseconds(1000);
  };

  /// @brief 
  struct async_launch {
    std::launch policy = std::launch::async;
  };

  /// @brief Object holding parameters for a generic launch
  using async_policy = std::variant<blocking_launch, async_launch>;

  template <typename T>
  class async_adaptor {
  private :
    using impl_type = T;
    impl_type _impl;

    template <typename Fn, typename ...Args>
    auto adapt(Fn fn, const async_policy& policy, auto&& ...args)
    {
      return std::visit(utils::visitor{
        [this](blocking_launch p) {}
      }, policy);
    }

    template <typename Fn, typename ...Args>
    auto launch_blocking(Fn fn, Args&& ...Args) {
      return std::async()
    }

  public :

    std::future<packet_status> send(const packet& p, const async_policy& policy = blocking_launch())
    {

    }

    std::future<packet_status> receive(const packet& p, const async_policy& policy = blocking_launch())
    {

    }

    void kill(const async_policy& policy)
    {
    }

    bool alive() const noexcept = 0
    { return _impl.alive(); }
  };
}