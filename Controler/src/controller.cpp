
#include "bridge.h"
#include "bridges/tcp.h"

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>

namespace bridge {
  class dummy_bridge : public bridge {
  public :

    std::future<packet::status> send(const packet& p) override
    { 
      std::cout << "sent " << p.payload.size() << " bytes to "
        << p.address << std::endl;
      auto& prm = tasks.emplace_back();
      return tasks.back().get_future();
    }

    std::optional<reply> receive() override
    {
      if (tasks.size() != 0)
        return std::make_optional<reply>(std::to_string(tasks.size()) + " Objects pending");
      else
        return std::nullopt;
    }

    void flush() { for (auto& t : tasks) t.set_value(packet::status::Sent); }
    void kill(bool async = false) override {}
    bool alive() const noexcept override { return true; }

    ~dummy_bridge() = default;

  private :
    std::vector<std::promise<packet::status>> tasks;
  };

  class dummy_async_bridge : public async_bridge {
  public :
    dummy_async_bridge() : async_bridge(std::jthread(
        [](std::stop_token stoken, dummy_async_bridge* bridge) -> void {
          while (!stoken.stop_requested())
          {
            {
              auto proxy = bridge->to_device();
              if (proxy._obj.empty())
                continue;
              std::cout << "Objects pending : " << proxy._obj.size() << "\n";
              auto& msg = proxy._obj.front();
              {
                auto rproxy = bridge->from_device();
                rproxy._obj.emplace(std::string("Sending object to ")
                  + std::to_string(msg.second.address) + " : "
                  + std::to_string(msg.second.payload.size()));
              }
              msg.first.set_value(packet::status::Sent);
              proxy._obj.pop();
              if (!proxy._obj.empty())
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
          }
      }, this)) 
      {}

    ~dummy_async_bridge() {
      kill();
    }
  private :
  };
}

int main(int argc, char * const argv[])
{
  using namespace std::chrono_literals;
  {
    bridge::dummy_bridge bridge;
    assert(bridge.alive());

    auto f = bridge.send(bridge::packet{19, {0, 1, 2, 0xFF}});
    assert(f.valid());

    auto r = bridge.receive();
    assert(r.has_value());
    std::cout << "Received : " << r.value().content << std::endl;
    
    assert(f.wait_for(0s) == std::future_status::timeout);
    bridge.flush();
    assert(f.wait_for(0s) == std::future_status::ready);
    assert(f.get() == bridge::packet::status::Sent);
  }
  {
    bridge::dummy_async_bridge abridge;
    assert(abridge.alive());

    std::vector<std::future<bridge::packet::status>> pendings;
    auto sender = std::async([&]() -> void {
      for (size_t i=0 ; i<10 ; ++i)
      {
        assert(abridge.alive());
        auto f = abridge.send(bridge::packet{i, {}});
        assert(f.valid());
        pendings.emplace_back(std::move(f));
        std::this_thread::sleep_for(15ms);
      }
    });

    auto receiver = std::async([&]() -> void {
      size_t index = 0;
      for (size_t i=0 ; i<20 ; ++i)
      {
        auto rsp = abridge.receive();
        if (rsp.has_value())
        {
          assert(pendings[index].valid());
          assert(pendings[index].wait_for(0s) == std::future_status::ready);
          std::cout << "Received : " << rsp.value().content << "\n";
          index += 1;
        }
        else
        {
          // may not pass on bad chance
          if (index < pendings.size())
            assert(pendings[index].wait_for(0s) == std::future_status::timeout);
        }
        std::this_thread::sleep_for(52ms);
      }
    });

    sender.wait();
    receiver.wait();
  }
  {
    using namespace bridge::tcp;
    tcp_bridge tbridge(bridge::address{"192.168.0.177", "8000"});
    std::this_thread::sleep_for(1000ms);

    for (int i=0 ; i<10 ; ++i)
    {
      std::cout << "Try send bytes to arduino\n";
      auto f = tbridge.send(bridge::packet{i, {(uint8_t)i, 'H', 'R', 'u', '\n', '\n'}});
      assert(f.valid());
      std::this_thread::sleep_for(500ms);
      assert(f.wait_for(0s) == std::future_status::ready);
    }

    for (int i=0 ; i<50 ; ++i)
    {
      auto rsp = tbridge.receive();
      if (rsp.has_value())
        std::cout << "Received from arduino : \n" << rsp.value().content << "\n";
      std::this_thread::sleep_for(100ms);
    }

    std::cout << "Close the connection\n";
  }

  return 0;
}