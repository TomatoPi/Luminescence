#pragma once

#include "transport/transport.h"
#include "transport/transports/tcp.h"
#include "transport/transports/serial.h"

#include "utils/visitor.h"

#include <utility>
#include <variant>
#include <memory>
#include <stdexcept>
#include <future>

#include <iostream>

/// @brief Main namespace for optopoulpe top objects
namespace opto {

  using transport_signature_type = std::variant<
    tcp::socket_signature,
    serial::serial_signature
    >;

  struct device_config {
    std::string name;
  };

  using device_signature = std::tuple<device_config, transport_signature_type>;

  class device {
  public :

    using transport_type = struct opto::transport;

    using transport_ptr     = std::unique_ptr<transport_type>;
    using pending_transport = std::future<transport_ptr>;
    using maybe_transport   = std::variant<transport_ptr, pending_transport>;

    explicit device(const device_signature& sig)
    : _cfg(std::get<device_config>(sig)),
      _sig(std::get<transport_signature_type>(sig)),
      _transport(open(_sig))
    {}

    device_config config() const noexcept
    { return _cfg; }

    void reload()
    {
      std::visit(utils::visitor{
        [this](transport_ptr&){
          std::cout << "[" << _cfg.name << "] - " << "Realoading device : " << _cfg.name << " ...\n";
          _transport = open(_sig);
        },
        [this](pending_transport& future) {
          if (future.wait_for(std::chrono::seconds(0)) != std::future_status::timeout)
          {
            auto ptr = future.get();
            _transport = std::move(ptr);
            std::cout << "[" << _cfg.name << "] - " << "Realoading device : " << "Done\n";
          }
        }
      }, _transport);
    }

    void kill()
    { _transport = nullptr; }


    bool alive() noexcept
    { 
      return enforce();
      // return std::visit(utils::visitor{
      // [](const transport_ptr& ptr) { return ptr != nullptr && ptr->alive(); },
      // [](const pending_transport&) { return false; }
      // }, _transport);
    }

    packet_status send(const packet_payload& p)
    try {
      if (enforce())
        return std::get<transport_ptr>(_transport)->send(p);
      else
        return packet_status::Failed;
    }
    catch (std::runtime_error& e)
    {
      reload();
      return packet_status::Failed;
    }

    opt_reply receive()
    try {
      if (enforce())
        return std::get<transport_ptr>(_transport)->receive();
      else
        return std::nullopt;
    }
    catch (std::runtime_error& e)
    {
      reload();
      return std::nullopt;
    }

  private :

    bool enforce() noexcept
    try 
    {
      return std::visit(utils::visitor{
        [this](transport_ptr& ptr){
          if (ptr == nullptr)
          {
            _transport = open(_sig);
            return false;
          }
          else
          {
            return true;
          }
        },
        [this](pending_transport& future) {
          if (future.wait_for(std::chrono::microseconds(10)) != std::future_status::timeout)
          {
            auto ptr = future.get();

            if (!ptr->alive())
              throw std::runtime_error("Bad device opened");
            if (opto::packet_status::Sent != ptr->send({0}))
              throw std::runtime_error("Failed to Bang Open connection");
            std::cout << "[" << _cfg.name << "] - " << "Realoading device : " <<  "Port successfully connected\n";

            _transport = std::move(ptr);
            return true;
          }
          else
          {
            return false;
          }
        }
      }, _transport);
    }
    catch (std::runtime_error& e)
    {
      std::cerr << "[" << _cfg.name << "] - " << "Realoading device : " << "Failed reload : " << e.what() << '\n';
      _transport = open(_sig);
      return false;
    }

    static maybe_transport open(const transport_signature_type& sig)
    try {
      return std::visit(utils::visitor{
        [](const tcp::socket_signature& s)
          -> pending_transport
          { 
            auto addr = std::get<tcp::address>(std::get<tcp::socket_config>(s));
            std::cout << "Open TCP Socket : " << addr.host << ":" << addr.port << "\n";
            return std::async(std::launch::async, [](const tcp::socket_signature& s)
              -> transport_ptr
              { return std::make_unique<tcp::socket>(s); }, s);
          }
          ,
        [](const serial::serial_signature& s)
          -> pending_transport
          {
            auto addr = std::get<serial::address>(std::get<serial::serial_config>(s));
            std::cout << "Open Serial Connection : " << addr.port << " " << addr.baudrate << "Bauds\n";
            return std::async(std::launch::async, [](const serial::serial_signature& s)
              -> transport_ptr 
              { return std::make_unique<serial::serial>(s); }, s);
          }
      }, sig);
    }
    catch (std::runtime_error& e)
    {
      std::cerr << "Failure : " << e.what() << '\n';
      return std::async(std::launch::deferred, []() -> transport_ptr { return nullptr; });
    }

    device_config             _cfg;
    transport_signature_type  _sig;
    maybe_transport           _transport;
  };

}