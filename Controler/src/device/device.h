#pragma once

#include "transport/transport.h"

#include "transport/transports/factory.h"

#include <tuple>
#include <utility>
#include <variant>
#include <memory>
#include <stdexcept>
#include <future>
#include <shared_mutex>
#include <queue>

#include <iostream>

/// @brief Main namespace for optopoulpe top objects
namespace opto {
namespace device {

  namespace meta {
    
    struct name { std::string value = "default-device"; };
    using transport = ::transport::factory::transport_signature_type;
  }

  using signature = std::tuple<meta::name, meta::transport>;
  
  class device {
  public :

    using payload = transport::packet_payload;
    using status = std::future<transport::packet_status>;
    using reply = transport::reply;
    using transport_ptr = ::transport::factory::transport_ptr;

    struct pending {
      std::promise<transport::packet_status> promise;
      payload datas;
    };
    
    device() = default;
    ~device() = default;

    device(const device&) = delete;
    device& operator= (const device&) = delete;

    device(device&& d) = delete;
    device& operator= (device&& s) = delete;

    explicit device(const signature& sig)
    : _cfg(sig)
    {
      auto transport = transport::factory::open(std::get<meta::transport>(sig));
      _transport_terminated = std::async(std::launch::async, 
        [](device* dev, transport_ptr transport, std::future<int> terminate) -> int {
          while (terminate.valid() && terminate.wait_for(std::chrono::seconds(0)) == std::future_status::timeout
            && transport->alive())
          {
            do {
              auto _(dev->lock());

              if (dev->_send_queue.empty())
                break;

              auto& packet = dev->_send_queue.front();
              packet.promise.set_value(transport->send(packet.datas));
              dev->_send_queue.pop();

            } while (false);

            std::this_thread::sleep_for(std::chrono::microseconds(1000));

            do {
              auto _(dev->lock());

              auto opt = transport->receive();
              if (!opt.has_value())
                break;

              dev->_receive_queue.push((opt.value()));
            } while (false);

            std::this_thread::sleep_for(std::chrono::microseconds(1000));
          } // while true
          return 0;
        }, this, std::move(transport), _terminate_transport.get_future());
    }

    explicit operator signature () const noexcept
    { return _cfg; }

    bool alive() noexcept
    {
      return _transport_terminated.valid()
        && _transport_terminated.wait_for(std::chrono::seconds(0)) == std::future_status::timeout
        ;
    }

    void kill() { _terminate_transport.set_value(0); }

    status send(const payload& p)
    {
      auto _(lock());
      _send_queue.push(pending{.datas = p});
      return _send_queue.back().promise.get_future();
    }

    transport::opt_reply receive()
    {
      auto _(lock());
      if (_receive_queue.empty())
        return std::nullopt;
      reply r = _receive_queue.front();
      _receive_queue.pop();
      return r;
    }

  private :

    std::lock_guard<std::shared_mutex> lock() { return std::lock_guard<std::shared_mutex>(_mutex); }
    
    signature     _cfg;
    std::queue<pending> _send_queue;
    std::queue<reply> _receive_queue;
    std::future<int> _transport_terminated;
    std::promise<int> _terminate_transport;
    std::shared_mutex _mutex;
  };

}
}