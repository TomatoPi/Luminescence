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

namespace bridge
{
  struct address {
    std::string host;
    std::string port;
  };

  struct packet {
    enum class status {
      Failed, Sent, Skipped
    };
      
    uint64_t address;
    std::vector<uint8_t> payload;
  };

  struct reply {
    std::string content;
  };

  class bridge {
  public:

    virtual std::future<packet::status> send(const packet& p) = 0;
    virtual std::optional<reply> receive() = 0;

    virtual void kill(bool async = false) = 0;
    virtual bool alive() const noexcept = 0;
    
    virtual ~bridge() = default;
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