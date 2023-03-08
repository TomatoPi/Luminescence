#pragma once

#include "transport.h"

#include <thread>

namespace transport {

  template <typename T>
  class async_transport : transport {
  public:
    using decored_type = T;

    using promise_type = std::promise<packet::status>;
    using up_queue_type = std::queue<std::pair<promise_type, packet>>;
    using down_queue_type = std::queue<reply>;
    using lock_type = std::lock_guard<std::mutex>;

    template <typename Queue>
    using locking_queue = utils::locked<Queue>;
    template <typename Queue>
    using queue_proxy = typename locking_queue<Queue>::proxy;

  private:

    decored_type _impl;
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
}