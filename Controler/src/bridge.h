#pragma once

#include "utils/lock.h"

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

#include <future>
#include <mutex>
#include <queue>
#include <thread>

#include <variant>

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
    /// @return The future packet status, which will be determined
    ///   when the bridge has processed given packet
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

  class async_bridge : bridge {
  public:
    using promise_type = std::promise<packet::status>;
    using up_queue_type = std::queue<std::pair<promise_type, packet>>;
    using down_queue_type = std::queue<reply>;
    using lock_type = std::lock_guard<std::mutex>;

    template <typename Queue>
    using locking_queue = utils::locked<Queue>;
    template <typename Queue>
    using queue_proxy = typename locking_queue<Queue>::proxy;

  private:

    locking_queue<up_queue_type> _to_device;
    locking_queue<down_queue_type> _from_device;
    std::jthread _sending_thread;

  public:

    template <typename ...Args>
    async_bridge(Args&&... args)
      : _to_device(), _from_device(), _sending_thread(std::forward<Args>(args)...)
      {}

    async_bridge(const async_bridge&) = delete;
    async_bridge& operator= (const async_bridge&) = delete;

    async_bridge(async_bridge&& o) 
    : _to_device(std::move(o._to_device)),
    _from_device(std::move(o._from_device)),
    _sending_thread(std::move(o._sending_thread))
    {}

    ~async_bridge() = default;

    // Interface with derived classes

    queue_proxy<up_queue_type>   to_device()   { return _to_device.lock(); }
    queue_proxy<down_queue_type> from_device() { return _from_device.lock(); }

    // Inherited from bridge

    std::future<packet::status> send(const packet& p) override
    {
      promise_type promise;
      auto future = promise.get_future();
      {
        auto proxy = this->to_device();
        proxy._obj.emplace(std::move(promise), p);
      }
      return future;
    }

    std::optional<reply> receive() override
    {
      auto proxy = this->from_device();
      if (proxy._obj.empty())
        return std::nullopt;
      else {
        auto rsp = proxy._obj.front();
        proxy._obj.pop();
        return std::make_optional<reply>(rsp);
      }
    }

    void kill(bool async = false) override { _sending_thread.request_stop(); if(!async) _sending_thread.join(); }
    bool alive() const noexcept override { return !_sending_thread.get_stop_token().stop_requested(); }
  };

  template <
    typename T,
    typename Signature = typename T::signature_type
    >
  class persistant_bridge_adaptor : public bridge {
  public :
    enum class status {
      Dead, Active, Pending
    };

    using impl_type = T;
    using impl_signature_type = Signature;

  private:
    using future_impl_type = std::future<impl_type>;
    using dead_impl_type = int; /// Placeholder object
    using opt_impl_type = std::variant<impl_type, future_impl_type, dead_impl_type>;

    template<class... Ts> struct visitor : Ts... { using Ts::operator()...; };

    impl_signature_type _sig;
    opt_impl_type       _impl;
    bool                _killed;

    static opt_impl_type open(const impl_signature_type& sig)
    {
      return std::async([](const impl_signature_type& sig) -> impl_type {
        return impl_type(sig);
      }, sig);
    }

    void try_relaunch()
    { if (!_killed) _impl = open(_sig); }

    bool enforce_state()
    {
      if (_killed)
        return false;
      
      switch (this->get_status())
      {
      case status::Dead :
        this->try_relaunch();
        return false;
      case status::Active :
        return true;
      case status::Pending :
      {
        auto& f = std::get<future_impl_type>(_impl);
        if (f.wait_for(std::chrono::seconds(0)) != std::future_status::timeout)
        {
          auto tmp = f.get();
          _impl = std::move(tmp);
          return true;
        }
        else
          return false;
      }
      }
    }

  public:
    
    persistant_bridge_adaptor(auto&& ...args)
    : _sig(std::forward<decltype(args)>(args)...), _impl(open(_sig)), _killed(false)
    {}

    status get_status() const noexcept
    {
      return std::visit(visitor{
        [](impl_type& impl) { return impl.alive() ? status::Active : status::Dead; },
        [](future_impl_type& future) { return future.valid() ? status::Pending : status::Dead; },
        [](dead_impl_type) { return status::Dead; }
      }, _impl);
    }

    std::future<packet::status> send(const packet& p) override
    {
      if (!enforce_state())
        return std::async(std::launch::deferred, [](){ return packet::status::Failed; });

      try {

      }
      catch (std::exception& e)
      {

      }
    }

    std::optional<reply> receive() override
    {
      if (!enforce_state())
        return std::nullopt;

      try {

      }
      catch (std::exception& e)
      {

      }
    }

    void kill(bool async = false) override
    {
      _killed = true;
      if (_impl.index() == 0)
        std::get<0>(_impl).kill(async);
      
      if (!async) // kill now -> destroy the current _impl
        _impl = dead_impl_type();
    }

    bool alive() const noexcept override
    {
      return !_killed
        && impl_type::validate(_sig)
        && !_impl.valueless_by_exception()
        && std::visit(visitor{
          [](impl_type& impl) { return impl.alive(); },
          [](future_impl_type& future) { return future.valid(); },
          [](dead_impl_type) { return false; }
          }, _impl);
    }
  };

  // enum class status {
  //   Dead, Ok
  // };

  // /// @brief Generic interface for leds drivers instances
  // /// @tparam Payload Type of messages sent to the driver
  // /// @tparam Ack Object returned by the bridge when a request is queued
  // /// @tparam Rsp 
  // template <typename Payload, typename Ack, typename Rsp>
  // class bridge {
  // public :
  //   using payload_type = Payload;
  //   using ack_type = Ack;
  //   using rsp_type = Rsp;

  //   std::vector<ack_type> send(std::ranges::range<payload_type> payloads)
  //   {
  //     std::vector<ack_type> acks;
  //     acks.reserve(payloads.size());
  //     for (const auto& msg : payloads)
  //       acks.push_back(this->send(msg));
  //     return acks;
  //   }
    
  //   virtual ack_type send(payload_type payload) = 0;
  //   virtual rsp_type recieve() = 0;

  //   virtual explicit operator bool () const noexcept = 0;
    
  //   virtual ~bridge() = 0;
  // };

  // template <typename Transport, typename Payload, typename Ack, typename Rsp>
  // struct driver : bridge<Payload, Ack, Rsp>
  // {
  //   using payload_type = Payload;
  //   using ack_type = Ack;
  //   using rsp_type = Rsp;

  //   using transport_type = Transport;

  //   template <typename... Args>
  //   static std::optional<driver> open(Args &&...args) noexcept
  //   {
  //     if (auto transport = Transport::open(std::forward<Args>(args)...); transport.has_value())
  //       return std::make_optional<driver>(std::move(transport.value()));
  //     else
  //       return std::nullopt;
  //   }

  //   std::variant<status, ack_type> send(payload_type&& payload) override
  //   { return transport.send(std::forward<payload_type>(payload)); }

  //   std::variant<status, rsp_type> receive() override
  //   { return transport.receive(); }

  //   explicit operator bool () const noexcept override
  //   { return static_cast<bool>(transport); }

  //   transport_type transport;
  // };

  // template <typename Container, typename Transport, typename Payload, typename Ack, typename Rsp>
  // struct driver_pool : bridge<Payload, Ack, Rsp>
  // {
  //   using payload_type = Payload;
  //   using ack_type = Ack;
  //   using rsp_type = Rsp;

  //   using driver_type = driver<Transport, Payload, Ack, Rsp>;
  //   using container_type = Container;

  //   template <typename ...Args>
  //   auto operator[] (Args&&... args) noexcept
  //   { return container.operator[](std::forward<Args>(args)...); }

  //   template <typename ...Args>
  //   auto at(Args&&... args) const noexcept
  //   { return container.at(std::forward<Args>(args)...); }

  //   std::variant<status, ack_type> send(payload_type payload) override
  //   {
  //     std::vector<
  //     for (driver_type& d : container)
  //     {
  //       try {
  //         auto ack = d.send(payload);

  //       }
  //       catch 
  //     }
  //   }

  //   explicit operator bool() const noexcept override
  //   { return true; }

  //   container_type container;
  // };
}